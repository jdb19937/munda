/*
 * nativus_cursus.c — passus ante transformatoris nativus
 *
 * Inferentiae et auxiliaria: alloc, I/O, status, forward pass.
 * Operationes matriciae per pfr_matvec_f (phantasma/computo.h).
 */

#include "nativus.h"

#include "phantasma/computo.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ================================================================
 * auxiliaria interna: BLAS wrappers (gpu=NULL -> CPU)
 * ================================================================ */

/*
 * matvec — y = W * x, W est (out, in), x est (in,), y est (out,).
 * Utitur pfr_matvec_f (phantasma API generalis).
 */
static void matvec(float *y, const float *W, const float *x, int out, int in)
{
    pfr_matrix_f_t A = { out, in, (float *)W, NULL };
    pfr_vector_f_t xv = { in,  (float *)x,  NULL };
    pfr_vector_f_t yv = { out, y,            NULL };
    pfr_matvec_f(&yv, &A, &xv);
}

/* ================================================================
 * primitiva activationum
 * ================================================================ */

static void rmsnorma(float *o, const float *x, const float *w, int n)
{
    float ss = 0.0f;
    for (int j = 0; j < n; j++) ss += x[j] * x[j];
    ss = 1.0f / sqrtf(ss / n + 1e-5f);
    for (int j = 0; j < n; j++) o[j] = w[j] * ss * x[j];
}

static void mollifica(float *x, int n) /* softmax in situ */
{
    float mx = x[0];
    for (int i = 1; i < n; i++) if (x[i] > mx) mx = x[i];
    float s = 0.0f;
    for (int i = 0; i < n; i++) { x[i] = expf(x[i] - mx); s += x[i]; }
    for (int i = 0; i < n; i++) x[i] /= s;
}

static void rope_rotatio(float *v, int pos, int caput_dim)
{
    for (int i = 0; i < caput_dim; i += 2) {
        float freq = 1.0f / powf(10000.0f, (float)i / caput_dim);
        float val  = pos * freq;
        float fcr  = cosf(val), fci = sinf(val);
        float v0 = v[i], v1 = v[i + 1];
        v[i]     = v0 * fcr - v1 * fci;
        v[i + 1] = v0 * fci + v1 * fcr;
    }
}

/* ================================================================
 * magnitudo ponderum
 * ================================================================ */

size_t nm_magnitudo_ponderum(const nm_config_t *c, int communes)
{
    int d = c->dimensio, df = c->dimensio_occ, L = c->strata;
    int V = c->vocab_magnitudo, H = c->capita, Hkv = c->capita_kv;
    int kv_dim = (d / H) * Hkv;
    size_t n = 0;
    n += (size_t)V * d;          /* vestigia */
    n += (size_t)L * d;          /* rms_att */
    n += (size_t)L * d;          /* rms_ffn */
    n += (size_t)L * d * d;      /* wq */
    n += (size_t)L * kv_dim * d; /* wk */
    n += (size_t)L * kv_dim * d; /* wv */
    n += (size_t)L * d * d;      /* wo */
    n += (size_t)L * df * d;     /* w1 */
    n += (size_t)L * d * df;     /* w2 */
    n += (size_t)L * df * d;     /* w3 */
    n += (size_t)d;              /* rms_finis */
    if (!communes) n += (size_t)V * d; /* wvoc */
    return n;
}

float *nm_pondera_init_ptr(nm_pondera_t *p, const nm_config_t *c, float *data)
{
    int d = c->dimensio, df = c->dimensio_occ, L = c->strata;
    int V = c->vocab_magnitudo, H = c->capita, Hkv = c->capita_kv;
    int kv_dim = (d / H) * Hkv;
    p->vestigia  = data; data += (size_t)V * d;
    p->rms_att   = data; data += (size_t)L * d;
    p->rms_ffn   = data; data += (size_t)L * d;
    p->wq        = data; data += (size_t)L * d * d;
    p->wk        = data; data += (size_t)L * kv_dim * d;
    p->wv        = data; data += (size_t)L * kv_dim * d;
    p->wo        = data; data += (size_t)L * d * d;
    p->w1        = data; data += (size_t)L * df * d;
    p->w2        = data; data += (size_t)L * d * df;
    p->w3        = data; data += (size_t)L * df * d;
    p->rms_finis = data; data += (size_t)d;
    p->wvoc      = NULL;
    return data;
}

/* ================================================================
 * alloca / libera status
 * ================================================================ */

static int status_alloca(nm_status_t *s, const nm_config_t *c)
{
    int d = c->dimensio, df = c->dimensio_occ, L = c->strata;
    int V = c->vocab_magnitudo, H = c->capita, Hkv = c->capita_kv;
    int kv_dim = (d / H) * Hkv;
    int lm = c->longitudo_max;
#define ALLOCA(campo, n) \
    s->campo = calloc((size_t)(n), sizeof(float)); \
    if (!s->campo) return -1;
    ALLOCA(x,    d)
    ALLOCA(xb,   d)
    ALLOCA(xb2,  d)
    ALLOCA(hb,   df)
    ALLOCA(hb2,  df)
    ALLOCA(q,    d)
    ALLOCA(k,    kv_dim)
    ALLOCA(v,    kv_dim)
    ALLOCA(att,  (size_t)H * lm)
    ALLOCA(logitae, V)
    ALLOCA(cache_clavis, (size_t)L * lm * kv_dim)
    ALLOCA(cache_valor,  (size_t)L * lm * kv_dim)
#undef ALLOCA
    return 0;
}

static void status_libera(nm_status_t *s)
{
    free(s->x);       free(s->xb);  free(s->xb2);
    free(s->hb);      free(s->hb2);
    free(s->q);       free(s->k);   free(s->v);
    free(s->att);     free(s->logitae);
    free(s->cache_clavis); free(s->cache_valor);
    memset(s, 0, sizeof(*s));
}

/* ================================================================
 * initia temere (He initialization)
 * ================================================================ */

/* LCG pro initializatione temeraria */
static float temerarius(unsigned int *semen)
{
    *semen = *semen * 1664525u + 1013904223u;
    /* uniformis in (-1, 1] */
    return ((float)(int)*semen) / 2147483648.0f;
}

int nm_initia_temere(nm_t *nm, const nm_config_t *config, unsigned int semen)
{
    memset(nm, 0, sizeof(*nm));
    nm->config = *config;
    nm->pondera_communes = 1;

    size_t n = nm_magnitudo_ponderum(config, 1);
    nm->magnitudo_data = n;
    nm->data = calloc(n, sizeof(float));
    if (!nm->data) return -1;

    nm_pondera_init_ptr(&nm->pondera, config, nm->data);
    nm->pondera.wvoc = nm->pondera.vestigia; /* communes */

    /* He initialization: sigma = sqrt(2/n_in) */
    int d = config->dimensio, df = config->dimensio_occ;
    int L = config->strata, V = config->vocab_magnitudo;
    int H = config->capita, Hkv = config->capita_kv;
    int kv_dim = (d / H) * Hkv;

    float sc_vestigia = sqrtf(2.0f / d);
    for (size_t i = 0; i < (size_t)V * d; i++)
        nm->pondera.vestigia[i] = temerarius(&semen) * sc_vestigia;

    /* rms weights: initia in 1 */
    for (int l = 0; l < L; l++) {
        for (int i = 0; i < d; i++) nm->pondera.rms_att[l*d + i] = 1.0f;
        for (int i = 0; i < d; i++) nm->pondera.rms_ffn[l*d + i] = 1.0f;
    }
    for (int i = 0; i < d; i++) nm->pondera.rms_finis[i] = 1.0f;

    /* attention weights: He init */
    float sc_att = sqrtf(2.0f / d);
    for (int l = 0; l < L; l++) {
        for (size_t i = 0; i < (size_t)d * d; i++) {
            nm->pondera.wq[l * d * d + i] = temerarius(&semen) * sc_att;
            nm->pondera.wo[l * d * d + i] = temerarius(&semen) * sc_att;
        }
        for (size_t i = 0; i < (size_t)kv_dim * d; i++) {
            nm->pondera.wk[l * kv_dim * d + i] = temerarius(&semen) * sc_att;
            nm->pondera.wv[l * kv_dim * d + i] = temerarius(&semen) * sc_att;
        }
    }

    /* FFN weights */
    float sc_ff = sqrtf(2.0f / d), sc_ff2 = sqrtf(2.0f / df);
    for (int l = 0; l < L; l++) {
        for (size_t i = 0; i < (size_t)df * d; i++) {
            nm->pondera.w1[l * df * d + i] = temerarius(&semen) * sc_ff;
            nm->pondera.w3[l * df * d + i] = temerarius(&semen) * sc_ff;
        }
        for (size_t i = 0; i < (size_t)d * df; i++)
            nm->pondera.w2[l * d * df + i] = temerarius(&semen) * sc_ff2;
    }

    return status_alloca(&nm->status, config);
}

/* ================================================================
 * serva / lege
 * ================================================================ */

int nm_serva(const nm_t *nm, const char *via)
{
    FILE *f = fopen(via, "wb");
    if (!f) return -1;
    unsigned int mag = NM_SIGNUM_MAGICUM;
    fwrite(&mag, sizeof(unsigned int), 1, f);
    fwrite(&nm->config, sizeof(nm_config_t), 1, f);
    fwrite(&nm->pondera_communes, sizeof(int), 1, f);
    size_t n = nm_magnitudo_ponderum(&nm->config, nm->pondera_communes);
    fwrite(nm->data, sizeof(float), n, f);
    fclose(f);
    return 0;
}

int nm_lege(nm_t *nm, const char *via)
{
    FILE *f = fopen(via, "rb");
    if (!f) return -1;

    unsigned int mag;
    if (fread(&mag, sizeof(unsigned int), 1, f) != 1 ||
        mag != NM_SIGNUM_MAGICUM) {
        fclose(f); return -1;
    }

    memset(nm, 0, sizeof(*nm));
    if (fread(&nm->config, sizeof(nm_config_t), 1, f) != 1 ||
        fread(&nm->pondera_communes, sizeof(int), 1, f) != 1) {
        fclose(f); return -1;
    }

    size_t n = nm_magnitudo_ponderum(&nm->config, nm->pondera_communes);
    nm->data = malloc(n * sizeof(float));
    nm->magnitudo_data = n;
    if (!nm->data) { fclose(f); return -1; }
    if (fread(nm->data, sizeof(float), n, f) != n) {
        free(nm->data); nm->data = NULL; fclose(f); return -1;
    }
    fclose(f);

    nm_pondera_init_ptr(&nm->pondera, &nm->config, nm->data);
    nm->pondera.wvoc = nm->pondera_communes
                     ? nm->pondera.vestigia
                     : (nm->pondera.rms_finis + nm->config.dimensio);

    return status_alloca(&nm->status, &nm->config);
}

void nm_fini(nm_t *nm)
{
    if (!nm) return;
    status_libera(&nm->status);
    free(nm->data);
    nm->data = NULL;
}

void nm_status_restitue(nm_t *nm)
{
    nm_config_t *c = &nm->config;
    int d = c->dimensio, L = c->strata;
    int H = c->capita, Hkv = c->capita_kv;
    int kv_dim = (d / H) * Hkv;
    int lm = c->longitudo_max, V = c->vocab_magnitudo;
    nm_status_t *s = &nm->status;

    memset(s->x,    0, (size_t)d * sizeof(float));
    memset(s->xb,   0, (size_t)d * sizeof(float));
    memset(s->xb2,  0, (size_t)d * sizeof(float));
    memset(s->hb,   0, (size_t)c->dimensio_occ * sizeof(float));
    memset(s->hb2,  0, (size_t)c->dimensio_occ * sizeof(float));
    memset(s->q,    0, (size_t)d * sizeof(float));
    memset(s->k,    0, (size_t)kv_dim * sizeof(float));
    memset(s->v,    0, (size_t)kv_dim * sizeof(float));
    memset(s->att,  0, (size_t)H * lm * sizeof(float));
    memset(s->logitae, 0, (size_t)V * sizeof(float));
    memset(s->cache_clavis, 0, (size_t)L * lm * kv_dim * sizeof(float));
    memset(s->cache_valor,  0, (size_t)L * lm * kv_dim * sizeof(float));
}

/* ================================================================
 * passus ante (forward pass)
 * ================================================================ */

/*
 * cursus_interna — corpus principale; sic tam nm_cursus quam
 * nm_cursus_memo utuntur eodem codice, solo ex != NULL ad memoizationem.
 */
static float *cursus_interna(nm_t *nm, nm_exercitatio_t *ex,
                              int signum, int positio)
{
    nm_config_t *c  = &nm->config;
    nm_pondera_t *p = &nm->pondera;
    nm_status_t  *s = &nm->status;

    int d     = c->dimensio;
    int df    = c->dimensio_occ;
    int L     = c->strata;
    int H     = c->capita;
    int Hkv   = c->capita_kv;
    int hd    = d / H;          /* caput_dim */
    int kv_dim = hd * Hkv;
    int lm    = c->longitudo_max;
    int V     = c->vocab_magnitudo;
    int kv_mul = H / Hkv;       /* capita Q per caput KV */

    /* 1. vestigia */
    memcpy(s->x, p->vestigia + (size_t)signum * d, (size_t)d * sizeof(float));

    if (ex) memcpy(ex->memo_x, s->x, (size_t)d * sizeof(float));

    /* 2. strata */
    for (int l = 0; l < L; l++) {
        float *W_rms_att = p->rms_att   + (size_t)l * d;
        float *W_rms_ffn = p->rms_ffn   + (size_t)l * d;
        float *W_wq      = p->wq        + (size_t)l * d * d;
        float *W_wk      = p->wk        + (size_t)l * kv_dim * d;
        float *W_wv      = p->wv        + (size_t)l * kv_dim * d;
        float *W_wo      = p->wo        + (size_t)l * d * d;
        float *W_w1      = p->w1        + (size_t)l * df * d;
        float *W_w2      = p->w2        + (size_t)l * d * df;
        float *W_w3      = p->w3        + (size_t)l * df * d;

        /* a. rmsnorm attention */
        rmsnorma(s->xb, s->x, W_rms_att, d);
        if (ex) memcpy(ex->memo_xb_att + (size_t)l * d, s->xb,
                       (size_t)d * sizeof(float));

        /* b. Q K V */
        matvec(s->q, W_wq, s->xb, d, d);
        matvec(s->k, W_wk, s->xb, kv_dim, d);
        matvec(s->v, W_wv, s->xb, kv_dim, d);

        /* c. RoPE */
        for (int h = 0; h < H; h++)
            rope_rotatio(s->q + h * hd, positio, hd);
        for (int h = 0; h < Hkv; h++)
            rope_rotatio(s->k + h * hd, positio, hd);

        if (ex) {
            memcpy(ex->memo_q + (size_t)l * d, s->q,
                   (size_t)d * sizeof(float));
            memcpy(ex->memo_k + (size_t)l * kv_dim, s->k,
                   (size_t)kv_dim * sizeof(float));
        }

        /* d. cache KV */
        size_t cache_off = ((size_t)l * lm + positio) * kv_dim;
        memcpy(s->cache_clavis + cache_off, s->k,
               (size_t)kv_dim * sizeof(float));
        memcpy(s->cache_valor  + cache_off, s->v,
               (size_t)kv_dim * sizeof(float));

        /* e. multi-head attention */
        memset(s->xb, 0, (size_t)d * sizeof(float));
        for (int h = 0; h < H; h++) {
            float *q_h  = s->q  + h * hd;
            float *att  = s->att + h * lm;
            float *xb_h = s->xb + h * hd;
            int    hkv  = h / kv_mul;

            for (int t = 0; t <= positio; t++) {
                float *k_t = s->cache_clavis +
                             ((size_t)l * lm + t) * kv_dim + hkv * hd;
                float sc = 0.0f;
                for (int i = 0; i < hd; i++) sc += q_h[i] * k_t[i];
                att[t] = sc / sqrtf((float)hd);
            }
            mollifica(att, positio + 1);

            for (int t = 0; t <= positio; t++) {
                float *v_t = s->cache_valor +
                             ((size_t)l * lm + t) * kv_dim + hkv * hd;
                float a = att[t];
                for (int i = 0; i < hd; i++) xb_h[i] += a * v_t[i];
            }
        }

        if (ex) memcpy(ex->memo_att + (size_t)l * H * lm, s->att,
                       (size_t)H * lm * sizeof(float));

        /* f. projection output + residuum */
        matvec(s->xb2, W_wo, s->xb, d, d);
        for (int i = 0; i < d; i++) s->x[i] += s->xb2[i];

        if (ex) memcpy(ex->memo_x + (size_t)(l + 1) * d, s->x,
                       (size_t)d * sizeof(float));

        /* g. rmsnorm FFN */
        rmsnorma(s->xb, s->x, W_rms_ffn, d);
        if (ex) memcpy(ex->memo_xb_ffn + (size_t)l * d, s->xb,
                       (size_t)d * sizeof(float));

        /* h. FFN w1, w3 */
        matvec(s->hb,  W_w1, s->xb, df, d);
        matvec(s->hb2, W_w3, s->xb, df, d);

        if (ex) {
            memcpy(ex->memo_hb  + (size_t)l * df, s->hb,
                   (size_t)df * sizeof(float));
            memcpy(ex->memo_hb2 + (size_t)l * df, s->hb2,
                   (size_t)df * sizeof(float));
        }

        /* i. SwiGLU: hb = silu(hb) * hb2 */
        for (int i = 0; i < df; i++) {
            float v0 = s->hb[i];
            float sig = 1.0f / (1.0f + expf(-v0));
            s->hb[i] = v0 * sig * s->hb2[i];
        }

        /* j. FFN w2 + residuum */
        matvec(s->xb, W_w2, s->hb, d, df);
        for (int i = 0; i < d; i++) s->x[i] += s->xb[i];
    }

    /* 3. rmsnorm finis */
    rmsnorma(s->x, s->x, p->rms_finis, d);
    if (ex) memcpy(ex->memo_xfin, s->x, (size_t)d * sizeof(float));

    /* 4. logitae */
    matvec(s->logitae, p->wvoc, s->x, V, d);
    return s->logitae;
}

float *nm_cursus(nm_t *nm, int signum, int positio)
{
    return cursus_interna(nm, NULL, signum, positio);
}

float *nm_cursus_memo(nm_t *nm, nm_exercitatio_t *ex, int signum, int positio)
{
    return cursus_interna(nm, ex, signum, positio);
}
