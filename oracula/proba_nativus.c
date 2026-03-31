/*
 * proba_nativus.c — probatio transformatoris nativus
 *
 * 1. Legit corpus ex oracula/corpus/dieta_1526_*.txt
 * 2. Exercitat lexatorem BPE
 * 3. Initiat exemplar parvum
 * 4. Exercitat per N passus, imprimat damnum
 * 5. Servat exemplar et lexatorem
 * 6. Legit exemplar novum
 * 7. Generat textum ex promptu
 * 8. Probat interfaciem provisoris nativus
 */

#include "nativus.h"
#include "nativus_lexator.h"
#include "phantasma/computo.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ================================================================
 * lectio corporis
 * ================================================================ */

static char *lege_plicam(const char *via)
{
    FILE *f = fopen(via, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long mag = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc((size_t)mag + 1);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, (size_t)mag, f);
    buf[mag] = '\0';
    fclose(f);
    return buf;
}

static char *lege_corpus(void)
{
    const char *plicae[] = {
        "oracula/corpus/dieta_1526_i.txt",
        "oracula/corpus/dieta_1526_ii.txt",
        "oracula/corpus/dieta_1526_iii.txt",
        "oracula/corpus/dieta_1526_iv.txt",
        "oracula/corpus/dieta_1526_v.txt",
        "oracula/corpus/dieta_1526_vi.txt",
    };
    int n_plicae = 6;

    /* summa magnitudine */
    size_t mag_tot = 0;
    char **partes = malloc((size_t)n_plicae * sizeof(char *));
    if (!partes) return NULL;
    for (int i = 0; i < n_plicae; i++) {
        partes[i] = lege_plicam(plicae[i]);
        if (partes[i]) mag_tot += strlen(partes[i]) + 1;
        else fprintf(stderr, "admonitio: non potui legere '%s'\n", plicae[i]);
    }

    char *corpus = malloc(mag_tot + 1);
    if (!corpus) {
        for (int i = 0; i < n_plicae; i++) free(partes[i]);
        free(partes);
        return NULL;
    }
    corpus[0] = '\0';
    for (int i = 0; i < n_plicae; i++) {
        if (partes[i]) {
            strcat(corpus, partes[i]);
            strcat(corpus, "\n");
            free(partes[i]);
        }
    }
    free(partes);
    return corpus;
}

/* ================================================================
 * exercitatio
 * ================================================================ */

#define N_PASSUS      800
#define LONGITUDO_SEQ  48
#define N_MINI          4       /* fenestrae per passum (mini-manipulus) */
#define GRADUS_DISC   3e-3f
#define BETA1         0.9f
#define BETA2         0.999f
#define EPSILON       1e-8f
#define DESICATIO     1e-2f
#define NUNTIA_PASSUS  50

static void exercita(nm_t *nm, nm_exercitatio_t *ex,
                     const int *signa_corp, int n_corp,
                     int n_passus)
{
    int lm = nm->config.longitudo_max;
    int seq_lon = LONGITUDO_SEQ < lm ? LONGITUDO_SEQ : lm - 1;
    unsigned int semen = 42u;
    int max_inc = n_corp - seq_lon - 2;
    if (max_inc <= 0) max_inc = 1;

    for (int passus = 0; passus < n_passus; passus++) {
        nm_gradientes_pone_nihil(ex, &nm->config);
        float damnum_tot = 0.0f;

        for (int mini = 0; mini < N_MINI; mini++) {
            semen = semen * 1664525u + 1013904223u;
            int inc = (int)(semen % (unsigned int)max_inc);

            nm_status_restitue(nm);
            for (int t = 0; t < seq_lon && inc + t + 1 < n_corp; t++) {
                int sig  = signa_corp[inc + t];
                int targ = signa_corp[inc + t + 1];
                nm_cursus_memo(nm, ex, sig, t);
                damnum_tot += nm_retropulsio(nm, ex, sig, targ, t);
            }
        }

        nm_gradientes_tonde(ex, &nm->config, 1.0f);
        nm_passus_adami(nm, ex, GRADUS_DISC, BETA1, BETA2, EPSILON, DESICATIO);

        if ((passus + 1) % NUNTIA_PASSUS == 0) {
            float damnum_med = damnum_tot / (float)(seq_lon * N_MINI);
            printf("  passus %4d/%d  damnum=%.4f  perplexitas=%.2f\n",
                   passus + 1, n_passus,
                   damnum_med, expf(damnum_med));
            fflush(stdout);
        }
    }
}

/* ================================================================
 * generatio textus
 * ================================================================ */

static void genera_et_imprime(nm_t *nm, nm_lexator_t *lex,
                               const char *promptus, int n_genera)
{
    nm_config_t *c = &nm->config;
    int V = c->vocab_magnitudo;

    int n_buf = (int)strlen(promptus) + 4;
    int *signa = malloc((size_t)n_buf * sizeof(int));
    if (!signa) return;
    int n_sig = 0;
    nm_lexator_disseca(lex, promptus, 1, 0, signa, &n_sig);

    nm_status_restitue(nm);
    float *logitae = NULL;
    for (int t = 0; t < n_sig && t < c->longitudo_max - 1; t++)
        logitae = nm_cursus(nm, signa[t], t);
    int pos = n_sig < c->longitudo_max ? n_sig : c->longitudo_max - 1;
    free(signa);

    printf("%s", promptus);
    fflush(stdout);

    unsigned int semen = 12345u;
    for (int i = 0; i < n_genera && pos < c->longitudo_max && logitae; i++) {
        /* sampling cum temperatura 0.9 */
        float *prob = malloc((size_t)V * sizeof(float));
        if (!prob) break;
        memcpy(prob, logitae, (size_t)V * sizeof(float));
        for (int j = 0; j < V; j++) prob[j] /= 0.9f;
        float mx = prob[0];
        for (int j = 1; j < V; j++) if (prob[j] > mx) mx = prob[j];
        float s = 0.0f;
        for (int j = 0; j < V; j++) { prob[j] = expf(prob[j] - mx); s += prob[j]; }
        for (int j = 0; j < V; j++) prob[j] /= s;
        semen = semen * 1664525u + 1013904223u;
        float r = ((float)(semen & 0x7fffffff)) / (float)0x7fffffff;
        float cdf = 0.0f;
        int prox = V - 1;
        for (int j = 0; j < V; j++) {
            cdf += prob[j];
            if (r < cdf) { prox = j; break; }
        }
        free(prob);
        if (prox == 2) break;
        printf("%s", nm_lexator_redde(lex, prox));
        fflush(stdout);
        logitae = nm_cursus(nm, prox, pos++);
    }
    printf("\n");
}

/* ================================================================
 * main
 * ================================================================ */

int main(void)
{
    printf("=== proba_nativus ===\n\n");

    /* initia computo (GPU vel CPU fallback) */
    int gpu = pfr_computo_initia();
    printf("computo: %s\n\n", gpu == 0 ? "GPU" : "CPU");

    /* --- 1. lege corpus --- */
    printf("1. lego corpus...\n");
    char *corpus = lege_corpus();
    if (!corpus || strlen(corpus) < 10) {
        fprintf(stderr, "corpus vac vel non inventum\n");
        return 1;
    }
    printf("   longitudo corporis: %zu characteres\n\n", strlen(corpus));

    /* --- 2. exercita lexatorem --- */
    printf("2. exercito lexatorem BPE (vocab_max=512)...\n");
    nm_lexator_t *lex = nm_lexator_crea(1024);
    if (!lex || nm_lexator_exercita(lex, corpus) < 0) {
        fprintf(stderr, "lexator exercitatio defecit\n");
        free(corpus);
        return 1;
    }
    printf("   vocab_magnitudo: %d\n", lex->vocab_magnitudo);

    /* tokeniza corpus */
    int corp_lon = (int)strlen(corpus);
    int *signa_corp = malloc((size_t)(corp_lon + 4) * sizeof(int));
    if (!signa_corp) { free(corpus); nm_lexator_destrue(lex); return 1; }
    int n_signa_corp = 0;
    nm_lexator_disseca(lex, corpus, 0, 0, signa_corp, &n_signa_corp);
    printf("   signa in corpore: %d\n\n", n_signa_corp);
    free(corpus);

    /* serva lexatorem */
    if (nm_lexator_serva(lex, "nativus_dieta_1526.lex") < 0)
        fprintf(stderr, "admonitio: lexator servari non potuit\n");
    else
        printf("   lexator servatus: nativus_dieta_1526.lex\n\n");

    /* --- 3. initia exemplar --- */
    printf("3. initio exemplar (dim=64, strata=4, capita=4, vocab=%d)...\n",
           lex->vocab_magnitudo);
    nm_config_t config = {
        .dimensio      = 64,
        .dimensio_occ  = 172,
        .strata        = 4,
        .capita        = 4,
        .capita_kv     = 4,
        .vocab_magnitudo = lex->vocab_magnitudo,
        .longitudo_max = 64,
    };
    nm_t nm;
    if (nm_initia_temere(&nm, &config, 42u) < 0) {
        fprintf(stderr, "nm_initia_temere defecit\n");
        free(signa_corp);
        nm_lexator_destrue(lex);
        return 1;
    }
    size_t n_param = nm_magnitudo_ponderum(&config, 1);
    printf("   parametri: %zu floats = %.1f KB\n\n",
           n_param, n_param * sizeof(float) / 1024.0);

    /* --- 4. exercita --- */
    printf("4. exercito (%d passus)...\n", N_PASSUS);
    nm_exercitatio_t ex;
    if (nm_exercitatio_initia(&ex, &config) < 0) {
        fprintf(stderr, "nm_exercitatio_initia defecit\n");
        nm_fini(&nm);
        free(signa_corp);
        nm_lexator_destrue(lex);
        return 1;
    }
    exercita(&nm, &ex, signa_corp, n_signa_corp, N_PASSUS);
    nm_exercitatio_fini(&ex);
    free(signa_corp);
    printf("\n");

    /* --- 5. serva --- */
    printf("5. servo exemplar...\n");
    if (nm_serva(&nm, "nativus_dieta_1526.bin") < 0)
        fprintf(stderr, "admonitio: nm_serva defecit\n");
    else
        printf("   exemplar servatum: nativus_dieta_1526.bin\n\n");
    nm_fini(&nm);

    /* --- 6. lege exemplar novum --- */
    printf("6. lego exemplar ex plica...\n");
    nm_t nm2;
    if (nm_lege(&nm2, "nativus_dieta_1526.bin") < 0) {
        fprintf(stderr, "nm_lege defecit\n");
        nm_lexator_destrue(lex);
        pfr_computo_fini();
        return 1;
    }
    printf("   exemplar lectum: dim=%d strata=%d vocab=%d\n\n",
           nm2.config.dimensio, nm2.config.strata,
           nm2.config.vocab_magnitudo);

    /* --- 7. inferentiae --- */
    printf("7. genero textum...\n");
    printf("   [promptus: \"Carolus Imperator\"]\n   ");
    genera_et_imprime(&nm2, lex, "Carolus Imperator", 80);
    printf("\n");
    printf("   [promptus: \"Diomedes\"]\n   ");
    genera_et_imprime(&nm2, lex, "Diomedes", 80);
    printf("\n");
    printf("   [promptus: \"avis\"]\n   ");
    genera_et_imprime(&nm2, lex, "avis", 80);
    printf("\n");

    /* --- 8. proba generationis directa --- */
    printf("8. probo generationem directam ex lexatore novo...\n");
    {
        nm_lexator_t *lex2 = nm_lexator_crea(NM_LEX_VOCAB_PRAEFIN);
        if (lex2 && nm_lexator_lege(lex2, "nativus_dieta_1526.lex") == 0) {
            printf("   [promptus: \"bestia\"]\n   ");
            genera_et_imprime(&nm2, lex2, "bestia", 64);
            nm_lexator_destrue(lex2);
        } else {
            printf("   lexator novus legi non potuit\n");
            if (lex2) nm_lexator_destrue(lex2);
        }
    }
    printf("\n");

    nm_fini(&nm2);
    nm_lexator_destrue(lex);
    pfr_computo_fini();

    printf("=== proba finita ===\n");
    return 0;
}
