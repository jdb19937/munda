/*
 * nativus_exercitatio.c — retropulsio et exercitatio transformatoris
 *
 * Backward pass (BPTT truncata per positionem) + optimizer AdamW.
 * Gradientes accumuli in ex->grad, deinde passus_adami updateat pondera.
 */

#include "nativus.h"
#include "phantasma/computo.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

/* ================================================================
 * auxiliaria interna — per pfr API (GPU ubi adest, CPU fallback)
 * ================================================================ */

/*
 * matvec_trans_accum — y += W^T * x  via pfr_matvec_trans_f
 * W est (out_dim, in_dim), x est (out_dim,), y est (in_dim,).
 */
static void matvec_trans_accum(
    float *y, const float *W,
    const float *x, int out_dim, int in_dim
) {
    pfr_matrix_f_t A  = { out_dim, in_dim, (float *)W, NULL };
    pfr_vector_f_t xv = { out_dim, (float *)x, NULL };
    float *tmp = calloc((size_t)in_dim, sizeof(float));
    if (!tmp)
        return;
    pfr_vector_f_t tv = { in_dim, tmp, NULL };
    pfr_matvec_trans_f(&tv, &A, &xv);
    for (int i = 0; i < in_dim; i++)
        y[i] += tmp[i];
    free(tmp);
}

/*
 * ger_accum — W += alpha * x * y^T  via pfr_ger_f
 * W est (m, n), x est (m,), y est (n,).
 */
static void ger_accum(
    float *W, float alpha,
    const float *x, const float *y, int m, int n
) {
    pfr_matrix_f_t A  = { m, n, W, NULL };
    pfr_vector_f_t xv = { m, (float *)x, NULL };
    pfr_vector_f_t yv = { n, (float *)y, NULL };
    pfr_ger_f(&A, alpha, &xv, &yv);
}

/*
 * rmsnorma_retro — gradientes pro RMSNorm.
 * d_x += grad_x, d_w += grad_w  (accumulative).
 * x: intratio normae, w: pondera normae, d_y: gradiens exitus.
 */
static void rmsnorma_retro(
    float *d_x, float *d_w,
    const float *d_y, const float *x,
    const float *w, int n
) {
    float ss = 0.0f;
    for (int i = 0; i < n; i++)
        ss += x[i] * x[i];
    float s = 1.0f / sqrtf(ss / n + 1e-5f);  /* = 1 / rms */

    for (int i = 0; i < n; i++)
        d_w[i] += d_y[i] * x[i] * s;

    float A = 0.0f;
    for (int j = 0; j < n; j++)
        A += d_y[j] * w[j] * x[j];

    float c = s * s * s / n;
    for (int j = 0; j < n; j++)
        d_x[j] += s * w[j] * d_y[j] - c * x[j] * A;
}

/*
 * softmax_retro — gradientes pro softmax.
 * d_in[i] = p[i] * (d_out[i] - dot(d_out, p)).
 */
static void softmax_retro(
    float *d_in, const float *d_out,
    const float *p, int n
) {
    float dot = 0.0f;
    for (int i = 0; i < n; i++)
        dot += d_out[i] * p[i];
    for (int i = 0; i < n; i++)
        d_in[i] = p[i] * (d_out[i] - dot);
}

/*
 * rope_retro — gradientes pro RoPE (rotatio inversa).
 */
static void rope_retro(float *d_v, int pos, int caput_dim)
{
    for (int i = 0; i < caput_dim; i += 2) {
        float freq = 1.0f / powf(10000.0f, (float)i / caput_dim);
        float val  = pos * freq;
        float fcr  = cosf(val), fci = sinf(val);
        float v0   = d_v[i], v1 = d_v[i + 1];
        /* rotatio inversa: angulus negativus */
        d_v[i]     = v0 * fcr + v1 * fci;
        d_v[i + 1] = -v0 * fci + v1 * fcr;
    }
}

/* ================================================================
 * alloca / libera exercitatio
 * ================================================================ */

static float *alloca_pondera_plana(
    const nm_config_t *c, int communes,
    nm_pondera_t *p
) {
    size_t n    = nm_magnitudo_ponderum(c, communes);
    float *data = calloc(n, sizeof(float));
    if (!data)
        return NULL;
    float *dopo = nm_pondera_init_ptr(p, c, data);
    if (communes)
        p->wvoc = p->vestigia;
    else
        p->wvoc = dopo; /* post rms_finis */
    return data;
}

int nm_exercitatio_initia(nm_exercitatio_t *ex, const nm_config_t *c)
{
    memset(ex, 0, sizeof(*ex));

    /* gradientes (communes = 1 ut in exemplari) */
    ex->grad_data = alloca_pondera_plana(c, 1, &ex->grad);
    if (!ex->grad_data)
        return -1;

    ex->impetus_data = alloca_pondera_plana(c, 1, &ex->impetus);
    if (!ex->impetus_data)
        return -1;

    ex->vis_data = alloca_pondera_plana(c, 1, &ex->vis);
    if (!ex->vis_data)
        return -1;

    int d    = c->dimensio;
    int df   = c->dimensio_occ;
    int L    = c->strata;
    int H    = c->capita;
    int Hkv  = c->capita_kv;
    int kv_d = (d / H) * Hkv;
    int lm   = c->longitudo_max;

#define AMEM(campo, n) \
    ex->campo = calloc((size_t)(n), sizeof(float)); \
    if (!ex->campo) return -1;
    AMEM(memo_x,      (size_t)(L + 1) * d)
    AMEM(memo_xb_att, (size_t)L * d)
    AMEM(memo_xb_ffn, (size_t)L * d)
    AMEM(memo_q,      (size_t)L * d)
    AMEM(memo_k,      (size_t)L * kv_d)
    AMEM(memo_att,    (size_t)L * H * lm)
    AMEM(memo_hb,     (size_t)L * df)
    AMEM(memo_hb2,    (size_t)L * df)
    AMEM(memo_xfin,   (size_t)d)
#undef AMEM
    return 0;
}

void nm_exercitatio_fini(nm_exercitatio_t *ex)
{
    free(ex->grad_data);
    free(ex->impetus_data);
    free(ex->vis_data);
    free(ex->memo_x);
    free(ex->memo_xb_att);
    free(ex->memo_xb_ffn);
    free(ex->memo_q);
    free(ex->memo_k);
    free(ex->memo_att);
    free(ex->memo_hb);
    free(ex->memo_hb2);
    free(ex->memo_xfin);
    memset(ex, 0, sizeof(*ex));
}

void nm_gradientes_pone_nihil(nm_exercitatio_t *ex, const nm_config_t *c)
{
    size_t n = nm_magnitudo_ponderum(c, 1);
    memset(ex->grad_data, 0, n * sizeof(float));
}

/* ================================================================
 * retropulsio — backward pass pro una positione
 * ================================================================ */

float nm_retropulsio(
    nm_t *nm, nm_exercitatio_t *ex,
    int signum, int signum_target, int positio
) {
    nm_config_t  *c = &nm->config;
    nm_pondera_t *p = &nm->pondera;
    nm_pondera_t *g = &ex->grad;

    int d      = c->dimensio;
    int df     = c->dimensio_occ;
    int L      = c->strata;
    int H      = c->capita;
    int Hkv    = c->capita_kv;
    int hd     = d / H;
    int kv_d   = hd * Hkv;
    int lm     = c->longitudo_max;
    int V      = c->vocab_magnitudo;
    int kv_mul = H / Hkv;

    /* --- buffers temporanei (in pila vel globales per plicam) --- */
    float *d_x   = calloc((size_t)d, sizeof(float));
    float *d_xb  = calloc((size_t)d, sizeof(float));
    float *d_hb  = calloc((size_t)df, sizeof(float));
    float *d_hb2 = calloc((size_t)df, sizeof(float));
    float *d_q   = calloc((size_t)d, sizeof(float));
    float *d_k   = calloc((size_t)kv_d, sizeof(float));
    float *d_v   = calloc((size_t)kv_d, sizeof(float));
    float *d_log = calloc((size_t)V, sizeof(float));
    if (
        !d_x || !d_xb || !d_hb || !d_hb2 ||
        !d_q || !d_k  || !d_v  || !d_log
    ) {
        free(d_x);
        free(d_xb);
        free(d_hb);
        free(d_hb2);
        free(d_q);
        free(d_k);
        free(d_v);
        free(d_log);
        return 0.0f;
    }

    /* 1. damnum (cross-entropy) e logitae: dL/d_log = softmax - one_hot */
    /* logitae iam continet softmax? No, nm_cursus redit logitae cruda. */
    /* Computemus softmax pro loss e logitae (servata in nm->status.logitae). */
    float *logitae = nm->status.logitae;

    memcpy(d_log, logitae, (size_t)V * sizeof(float));
    {
        pfr_vector_f_t dv = {
            V, d_log, NULL
        };
        pfr_softmax_f(&dv);
    }
    float damnum = -logf(d_log[signum_target] + 1e-10f);
    d_log[signum_target] -= 1.0f;  /* gradiens: softmax - one_hot */

    /* 2. retro per wvoc: d_xfin = wvoc^T * d_log */
    memset(d_x, 0, (size_t)d * sizeof(float));
    matvec_trans_accum(d_x, p->wvoc, d_log, V, d);
    ger_accum(g->wvoc, 1.0f, d_log, ex->memo_xfin, V, d);
    /* (wvoc == vestigia per communes; grad.wvoc == grad.vestigia) */

    /* 3. retro per rmsnorm finis */
    float *d_x_retro = calloc((size_t)d, sizeof(float));
    if (!d_x_retro) {
        free(d_x);
        free(d_xb);
        free(d_hb);
        free(d_hb2);
        free(d_q);
        free(d_k);
        free(d_v);
        free(d_log);
        return damnum;
    }
    /* x ante rmsnorm finis = memo_x[(L)] */
    float *x_ante_finis = ex->memo_x + (size_t)L * d;
    rmsnorma_retro(d_x_retro, g->rms_finis, d_x, x_ante_finis, p->rms_finis, d);
    memcpy(d_x, d_x_retro, (size_t)d * sizeof(float));

    /* 4. retro per strata (in ordine inverso) */
    for (int l = L - 1; l >= 0; l--) {
        float *W_rms_att = p->rms_att   + (size_t)l * d;
        float *W_rms_ffn = p->rms_ffn   + (size_t)l * d;
        float *W_wq      = p->wq        + (size_t)l * d * d;
        float *W_wk      = p->wk        + (size_t)l * kv_d * d;
        float *W_wv      = p->wv        + (size_t)l * kv_d * d;
        float *W_wo      = p->wo        + (size_t)l * d * d;
        float *W_w1      = p->w1        + (size_t)l * df * d;
        float *W_w2      = p->w2        + (size_t)l * d * df;
        float *W_w3      = p->w3        + (size_t)l * df * d;

        float *G_rms_att = g->rms_att   + (size_t)l * d;
        float *G_rms_ffn = g->rms_ffn   + (size_t)l * d;
        float *G_wq      = g->wq        + (size_t)l * d * d;
        float *G_wk      = g->wk        + (size_t)l * kv_d * d;
        float *G_wv      = g->wv        + (size_t)l * kv_d * d;
        float *G_wo      = g->wo        + (size_t)l * d * d;
        float *G_w1      = g->w1        + (size_t)l * df * d;
        float *G_w2      = g->w2        + (size_t)l * d * df;
        float *G_w3      = g->w3        + (size_t)l * df * d;

        float *xb_ffn = ex->memo_xb_ffn + (size_t)l * d;
        float *hb_m   = ex->memo_hb     + (size_t)l * df;
        float *hb2_m  = ex->memo_hb2    + (size_t)l * df;
        float *x_in   = ex->memo_x      + (size_t)l * d; /* ante stratum l */
        float *x_mid  = ex->memo_x      + (size_t)(l+1) * d; /* post att res */

        /* --- retropulsio FFN --- */
        /* d_x e strato l+1 est gradiens post residuum FFN:
           x[l+1] = x_mid + ffn_out => d_x_mid += d_x, d_ffn_out = d_x */
        float *d_ffn = calloc((size_t)d, sizeof(float));
        if (!d_ffn)
            break;
        memcpy(d_ffn, d_x, (size_t)d * sizeof(float));

        /* retro per w2: d_hb = w2^T * d_ffn (gradiens ad hb_post = SwiGLU output) */
        memset(d_hb, 0, (size_t)df * sizeof(float));
        matvec_trans_accum(d_hb, W_w2, d_ffn, d, df);
        /* G_w2 += d_ffn * hb_post^T  ubi hb_post = silu(hb_m) * hb2_m */
        {
            float *hb_post = calloc((size_t)df, sizeof(float));
            if (hb_post) {
                pfr_vector_f_t ov = { df, hb_post,       NULL };
                pfr_vector_f_t av = { df, (float *)hb_m,  NULL };
                pfr_vector_f_t bv = { df, (float *)hb2_m, NULL };
                pfr_swiglu_f(&ov, &av, &bv);
                ger_accum(G_w2, 1.0f, d_ffn, hb_post, d, df);
                free(hb_post);
            }
        }

        /* retro per SwiGLU: hb_m = memo_hb = W1 @ xb (PRE-SwiGLU, non opus recomputo) */
        memset(d_hb2, 0, (size_t)df * sizeof(float));
        for (int i = 0; i < df; i++) {
            float sig       = 1.0f / (1.0f + expf(-hb_m[i]));
            float silu_val  = hb_m[i] * sig;
            float silu_grad = sig * (1.0f + hb_m[i] * (1.0f - sig));
            d_hb2[i]        = d_hb[i] * silu_val;
            d_hb[i]         = d_hb[i] * hb2_m[i] * silu_grad;
        }

        /* retro per w1: d_xb_ffn += w1^T * d_hb */
        memset(d_xb, 0, (size_t)d * sizeof(float));
        matvec_trans_accum(d_xb, W_w1, d_hb,  df, d);
        ger_accum(G_w1, 1.0f, d_hb, xb_ffn, df, d);

        /* retro per w3: d_xb_ffn += w3^T * d_hb2 */
        matvec_trans_accum(d_xb, W_w3, d_hb2, df, d);
        ger_accum(G_w3, 1.0f, d_hb2, xb_ffn, df, d);

        free(d_ffn);

        /* retro per rmsnorm FFN */
        memset(d_x_retro, 0, (size_t)d * sizeof(float));
        rmsnorma_retro(d_x_retro, G_rms_ffn, d_xb, x_mid, W_rms_ffn, d);
        /* d_x_mid += d_x_retro (residuum: x_mid intrat in rmsnorm FFN et
           etiam ut residuum: x[l+1] = x_mid + ffn_out) */
        for (int i = 0; i < d; i++)
            d_x[i] = d_x[i] + d_x_retro[i];

        /* --- retropulsio attention --- */
        float *xb_att = ex->memo_xb_att + (size_t)l * d;
        float *q_m    = ex->memo_q      + (size_t)l * d;
        float *att_m  = ex->memo_att    + (size_t)l * H * lm;

        /* d_att_out = d_x (gradiens residuo attention)
           x_mid = x_in + att_out => d_x_in = d_x_mid, d_att_out = d_x_mid */
        /* Prima: retro per wo */
        float *d_att_out = calloc((size_t)d, sizeof(float));
        if (!d_att_out)
            break;
        memcpy(d_att_out, d_x, (size_t)d * sizeof(float));

        /* retro per wo: d_xb_concat = wo^T * d_att_out */
        float *d_xb_concat = calloc((size_t)d, sizeof(float));
        if (!d_xb_concat) {
            free(d_att_out);
            break;
        }
        matvec_trans_accum(d_xb_concat, W_wo, d_att_out, d, d);
        /* ad computandum gradientem wo, opus est xb ANTE wo.
           in passu ante: xb = summa ponderata valorum.
           non memoizavimus directe; recomponimus ex att_m et cache_valor. */
        {
            float *xb_pre_wo = calloc((size_t)d, sizeof(float));
            if (xb_pre_wo) {
                for (int h = 0; h < H; h++) {
                    float *xh    = xb_pre_wo + h * hd;
                    int hkv      = h / kv_mul;
                    float *att_h = att_m + h * lm;
                    for (int t = 0; t <= positio; t++) {
                        float *v_t = nm->status.cache_valor +
                            ((size_t)l * lm + t) * kv_d + hkv * hd;
                        float a = att_h[t];
                        for (int i = 0; i < hd; i++)
                            xh[i] += a * v_t[i];
                    }
                }
                ger_accum(G_wo, 1.0f, d_att_out, xb_pre_wo, d, d);
                free(xb_pre_wo);
            }
        }

        /* retropulsio per attentionem caput per caput */
        memset(d_q, 0, (size_t)d * sizeof(float));
        memset(d_k, 0, (size_t)kv_d * sizeof(float));
        memset(d_v, 0, (size_t)kv_d * sizeof(float));

        float inv_sqrt_hd = 1.0f / sqrtf((float)hd);

        for (int h = 0; h < H; h++) {
            float *q_h   = q_m    + h * hd;
            float *d_q_h = d_q    + h * hd;
            float *att_h = att_m  + h * lm;
            float *d_c_h = d_xb_concat + h * hd;
            int hkv      = h / kv_mul;

            /* d_v[hkv] += sum_t att_h[t] * d_c_h */
            for (int t = 0; t <= positio; t++) {
                float *d_v_hkv = d_v + hkv * hd;
                float a        = att_h[t];
                for (int i = 0; i < hd; i++)
                    d_v_hkv[i] += a * d_c_h[i];
            }

            /* d_att_h[t] = dot(d_c_h, v_t) */
            float *d_att_score = calloc((size_t)(positio + 1), sizeof(float));
            if (d_att_score) {
                for (int t = 0; t <= positio; t++) {
                    float *v_t = nm->status.cache_valor +
                        ((size_t)l * lm + t) * kv_d + hkv * hd;
                    float sc = 0.0f;
                    for (int i = 0; i < hd; i++)
                        sc += d_c_h[i] * v_t[i];
                    d_att_score[t] = sc;
                }

                /* softmax retro */
                float *d_score_pre = calloc((size_t)(positio + 1), sizeof(float));
                if (d_score_pre) {
                    softmax_retro(d_score_pre, d_att_score, att_h, positio + 1);
                    /* scala per inv_sqrt_hd */
                    for (int t = 0; t <= positio; t++)
                        d_score_pre[t] *= inv_sqrt_hd;

                    /* d_q_h += sum_t d_score_pre[t] * k_t */
                    for (int t = 0; t <= positio; t++) {
                        float *k_t = nm->status.cache_clavis +
                            ((size_t)l * lm + t) * kv_d + hkv * hd;
                        float dsp = d_score_pre[t];
                        for (int i = 0; i < hd; i++)
                            d_q_h[i] += dsp * k_t[i];
                    }

                    /* d_k[pos, hkv] += d_score_pre[pos] * q_h (positio currens solum) */
                    {
                        float *d_k_hkv = d_k + hkv * hd;
                        float dsp      = d_score_pre[positio];
                        for (int i = 0; i < hd; i++)
                            d_k_hkv[i] += dsp * q_h[i];
                    }

                    free(d_score_pre);
                }
                free(d_att_score);
            }
        }

        free(d_att_out);
        free(d_xb_concat);

        /* RoPE retro in d_q et d_k */
        for (int h = 0; h < H; h++)
            rope_retro(d_q + h * hd, positio, hd);
        for (int h = 0; h < Hkv; h++)
            rope_retro(d_k + h * hd, positio, hd);

        /* retro per wq, wk, wv: d_xb_att est gradiens cumulatus */
        float *d_xb_att = calloc((size_t)d, sizeof(float));
        if (d_xb_att) {
            matvec_trans_accum(d_xb_att, W_wq, d_q, d, d);
            ger_accum(G_wq, 1.0f, d_q, xb_att, d, d);

            matvec_trans_accum(d_xb_att, W_wk, d_k, kv_d, d);
            ger_accum(G_wk, 1.0f, d_k, xb_att, kv_d, d);

            matvec_trans_accum(d_xb_att, W_wv, d_v, kv_d, d);
            ger_accum(G_wv, 1.0f, d_v, xb_att, kv_d, d);

            /* retro per rmsnorm att */
            memset(d_x_retro, 0, (size_t)d * sizeof(float));
            rmsnorma_retro(
                d_x_retro, G_rms_att, d_xb_att,
                x_in, W_rms_att, d
            );
            for (int i = 0; i < d; i++)
                d_x[i] = d_x[i] + d_x_retro[i];

            free(d_xb_att);
        }
    }

    /* 5. gradiente vestigia */
    for (int i = 0; i < d; i++)
        g->vestigia[(size_t)signum * d + i] += d_x[i];

    ex->signa_processata++;

    free(d_x);
    free(d_xb);
    free(d_hb);
    free(d_hb2);
    free(d_q);
    free(d_k);
    free(d_v);
    free(d_log);
    free(d_x_retro);

    return damnum;
}

/* ================================================================
 * tonsio gradientium (gradient clipping)
 * ================================================================ */

float nm_gradientes_tonde(
    nm_exercitatio_t *ex, const nm_config_t *c,
    float max_norma
) {
    size_t n    = nm_magnitudo_ponderum(c, 1);
    float *g    = ex->grad_data;
    float norma = 0.0f;
    for (size_t i = 0; i < n; i++)
        norma += g[i] * g[i];
    norma = sqrtf(norma);
    if (norma > max_norma) {
        float scala = max_norma / norma;
        for (size_t i = 0; i < n; i++)
            g[i] *= scala;
    }
    return norma;
}

/* ================================================================
 * passus AdamW
 * ================================================================ */

void nm_passus_adami(
    nm_t *nm, nm_exercitatio_t *ex,
    float gradus_discendi, float beta1, float beta2,
    float epsilon, float desicatio
) {
    ex->passus++;
    float t   = (float)ex->passus;
    float bc1 = 1.0f - powf(beta1, t);
    float bc2 = 1.0f - powf(beta2, t);

    size_t n = nm_magnitudo_ponderum(&nm->config, nm->pondera_communes);
    float *w = nm->data;
    float *g = ex->grad_data;
    float *m = ex->impetus_data;
    float *v = ex->vis_data;

    for (size_t i = 0; i < n; i++) {
        float gi     = g[i];
        m[i]         = beta1 * m[i] + (1.0f - beta1) * gi;
        v[i]         = beta2 * v[i] + (1.0f - beta2) * gi * gi;
        float m_corr = m[i] / bc1;
        float v_corr = v[i] / bc2;
        w[i] -= gradus_discendi * (
            m_corr / (sqrtf(v_corr) + epsilon)
            + desicatio * w[i]
        );
    }

    nm_gradientes_pone_nihil(ex, &nm->config);
}
