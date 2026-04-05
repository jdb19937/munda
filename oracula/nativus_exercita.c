/*
 * nativus_exercita.c — exercitatio transformatoris nativus ex linea mandati
 *
 * Usus: nativus_exercita <nomen> <corpus_via> [n_passus] [dim] [strata] [capita]
 *
 *   <nomen>       nomen exemplaris; servatur in <nomen>.bin et <nomen>.lex
 *   <corpus_via>  plica corporis (UTF-8)
 *   [n_passus]    passus exercitationis (praefin. 1000)
 *   [dim]         dimensio exemplaris (praefin. 128)
 *   [strata]      strata transformatoris (praefin. 4)
 *   [capita]      capita attentionis (praefin. 4)
 *
 * Si <nomen>.bin et <nomen>.lex iam adsunt, exercitatio continuatur
 * (pondera vetera onerantur; passus novi accumulantur).
 * Tutela (checkpoint) fit singulis TUTELA_PASSUS passibus.
 */

#include "nativus.h"
#include "nativus_lexator.h"
#include "computo.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ================================================================
 * parametri praefiniti
 * ================================================================ */

#define DEF_VOCAB_MAX    1024
#define DEF_DIMENSIO     128
#define DEF_STRATA       4
#define DEF_CAPITA       4
#define DEF_CAPITA_KV    2
#define DEF_LONGIT_MAX   128
#define DEF_N_PASSUS     1000
#define DEF_SEQ_LON      64
#define DEF_N_MINI       4        /* fenestrae per passum (mini-manipulus) */
#define DEF_GRADUS_MAX   1e-4f   /* gradus discendi maximus (initio) */
#define DEF_GRADUS_MIN   5e-6f   /* gradus discendi minimus (fine) */
#define DEF_BETA1        0.9f
#define DEF_BETA2        0.999f
#define DEF_EPSILON      1e-8f
#define DEF_DESICATIO    1e-3f
#define NUNTIA_PASSUS    100
#define TUTELA_PASSUS    500      /* salva singulis N passibus */

/* ================================================================
 * lectio plicae
 * ================================================================ */

static char *lege_plicam(const char *via)
{
    FILE *f = fopen(via, "rb");
    if (!f)
        return NULL;
    fseek(f, 0, SEEK_END);
    long mag = ftell(f);
    rewind(f);
    char *buf = malloc((size_t)mag + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    if (fread(buf, 1, (size_t)mag, f) != (size_t)mag) {
        free(buf);
        fclose(f);
        return NULL;
    }
    buf[mag] = '\0';
    fclose(f);
    return buf;
}

/* ================================================================
 * exercitatio — p_semen: status RNG inter tutelam conservatus
 * ================================================================ */

static void exercita(
    nm_t *nm, nm_exercitatio_t *ex,
    const int *signa_corp, int n_corp,
    int n_passus, int seq_lon,
    int passus_inicio, unsigned int *p_semen
) {
    int lm = nm->config.longitudo_max;
    if (seq_lon > lm - 1)
        seq_lon = lm - 1;
    int max_inc = n_corp - seq_lon - 2;
    if (max_inc <= 0)
        max_inc = 1;

    for (int passus = 0; passus < n_passus; passus++) {
        nm_gradientes_pone_nihil(ex, &nm->config);
        float damnum_tot = 0.0f;

        for (int mini = 0; mini < DEF_N_MINI; mini++) {
            *p_semen = *p_semen * 1664525u + 1013904223u;
            int inc  = (int)(*p_semen % (unsigned int)max_inc);

            nm_status_restitue(nm);
            for (int t = 0; t < seq_lon && inc + t + 1 < n_corp; t++) {
                int sig  = signa_corp[inc + t];
                int targ = signa_corp[inc + t + 1];
                nm_cursus_memo(nm, ex, sig, t);
                damnum_tot += nm_retropulsio(nm, ex, sig, targ, t);
            }
        }

        nm_gradientes_tonde(ex, &nm->config, 1.0f);
        /* cosine annealing: gradus decrescit de max ad min */
        int p_abs = passus_inicio + passus;
        int p_tot = passus_inicio + n_passus;
        float lr = DEF_GRADUS_MIN + 0.5f * (DEF_GRADUS_MAX - DEF_GRADUS_MIN)
        * (1.0f + cosf(3.14159265f * (float)p_abs / (float)p_tot));
        nm_passus_adami(
            nm, ex, lr, DEF_BETA1, DEF_BETA2,
            DEF_EPSILON, DEF_DESICATIO
        );

        if ((passus_inicio + passus + 1) % NUNTIA_PASSUS == 0) {
            float dm = damnum_tot / (float)(seq_lon * DEF_N_MINI);
            printf(
                "  passus %5d  damnum=%.4f  perplexitas=%.2f\n",
                passus_inicio + passus + 1, dm, expf(dm)
            );
            fflush(stdout);
        }
    }
}

/* ================================================================
 * main
 * ================================================================ */

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(
            stderr,
            "usus: %s <nomen> <corpus_via> [n_passus] [dim] [strata] [capita]\n"
            "  praefinita: n_passus=%d dim=%d strata=%d capita=%d kv=%d\n"
            "  si <nomen>.bin adest, exercitatio continuatur\n",
            argv[0], DEF_N_PASSUS, DEF_DIMENSIO, DEF_STRATA,
            DEF_CAPITA, DEF_CAPITA_KV
        );
        return 1;
    }
    const char *nomen      = argv[1];
    const char *corpus_via = argv[2];
    int n_passus  = argc >= 4 ? atoi(argv[3]) : DEF_N_PASSUS;
    int dim       = argc >= 5 ? atoi(argv[4]) : DEF_DIMENSIO;
    int strata    = argc >= 6 ? atoi(argv[5]) : DEF_STRATA;
    int capita    = argc >= 7 ? atoi(argv[6]) : DEF_CAPITA;
    int capita_kv = (capita >= 4) ? capita / 2 : DEF_CAPITA_KV;

    printf("=== nativus_exercita: %s ===\n\n", nomen);

    int gpu = pfr_computo_initia();
    printf("computo: %s\n\n", gpu == 0 ? "GPU" : "CPU");

    char via_bin[512], via_lex[512];
    snprintf(via_bin, sizeof(via_bin), "%s.bin", nomen);
    snprintf(via_lex, sizeof(via_lex), "%s.lex", nomen);

    /* --- 1. lege corpus --- */
    printf("1. lego corpus ex '%s'...\n", corpus_via);
    char *corpus = lege_plicam(corpus_via);
    if (!corpus || strlen(corpus) < 10) {
        fprintf(stderr, "erratum: corpus vacuum vel non inventum\n");
        pfr_computo_fini();
        return 1;
    }
    printf("   longitudo: %zu characteres\n\n", strlen(corpus));

    /* --- 2. lexator: tenta onerare vel exercita novum --- */
    nm_lexator_t *lex = nm_lexator_crea(DEF_VOCAB_MAX);
    if (!lex) {
        fprintf(stderr, "erratum: nm_lexator_crea defecit\n");
        free(corpus);
        pfr_computo_fini();
        return 1;
    }

    int resume = 0;
    {
        FILE *fp = fopen(via_lex, "rb");
        if (fp) { fclose(fp); }
        if (fp && nm_lexator_lege(lex, via_lex) == 0) {
            printf(
                "2. lexator oneratus ex '%s' (vocab=%d)\n\n",
                via_lex, lex->vocab_magnitudo
            );
            resume = 1;
        } else {
            printf("2. exercito lexatorem BPE (vocab_max=%d)...\n", DEF_VOCAB_MAX);
            if (nm_lexator_exercita(lex, corpus) < 0) {
                fprintf(stderr, "erratum: lexator exercitatio defecit\n");
                free(corpus);
                nm_lexator_destrue(lex);
                pfr_computo_fini();
                return 1;
            }
            printf("   vocab_magnitudo: %d\n", lex->vocab_magnitudo);
            if (nm_lexator_serva(lex, via_lex) < 0)
                fprintf(stderr, "admonitio: lexator servari non potuit\n");
            else
                printf("   lexator servatus: %s\n\n", via_lex);
        }
    }

    int corp_lon = (int)strlen(corpus);
    int *signa   = malloc((size_t)(corp_lon + 4) * sizeof(int));
    if (!signa) {
        free(corpus);
        nm_lexator_destrue(lex);
        pfr_computo_fini();
        return 1;
    }
    int n_signa = 0;
    nm_lexator_disseca(lex, corpus, 0, 0, signa, &n_signa);
    printf("   signa in corpore: %d\n\n", n_signa);
    free(corpus);

    /* --- 3. exemplar: onus vel initia novum --- */
    nm_t nm;
    if (resume && nm_lege(&nm, via_bin) == 0) {
        printf(
            "3. exemplar oneratum ex '%s' (dim=%d strata=%d vocab=%d)\n\n",
            via_bin, nm.config.dimensio, nm.config.strata,
            nm.config.vocab_magnitudo
        );
        /* recalibra dim/strata/capita ex exemplari onerato */
        dim       = nm.config.dimensio;
        strata    = nm.config.strata;
        capita    = nm.config.capita;
        capita_kv = nm.config.capita_kv;
    } else {
        resume  = 0;
        int occ = (dim * 8 / 3 + 7) & ~7;
        nm_config_t config = {
            .dimensio        = dim,
            .dimensio_occ    = occ,
            .strata          = strata,
            .capita          = capita,
            .capita_kv       = capita_kv,
            .vocab_magnitudo = lex->vocab_magnitudo,
            .longitudo_max   = DEF_LONGIT_MAX,
        };
        printf(
            "3. initio exemplar novum (dim=%d occ=%d strata=%d capita=%d"
            " kv=%d vocab=%d lm=%d)...\n",
            dim, occ, strata, capita, capita_kv,
            lex->vocab_magnitudo, DEF_LONGIT_MAX
        );
        if (nm_initia_temere(&nm, &config, 42u) < 0) {
            fprintf(stderr, "erratum: nm_initia_temere defecit\n");
            free(signa);
            nm_lexator_destrue(lex);
            pfr_computo_fini();
            return 1;
        }
        size_t n_param = nm_magnitudo_ponderum(&config, 1);
        printf(
            "   parametri: %zu floats = %.1f KB\n\n",
            n_param, n_param * sizeof(float) / 1024.0
        );
    }

    /* --- 4. exercita in segmentis cum tutela --- */
    printf(
        "4. exercito (%d passus, seq_lon=%d, mini=%d, tutela=%d)...\n",
        n_passus, DEF_SEQ_LON, DEF_N_MINI, TUTELA_PASSUS
    );
    nm_exercitatio_t ex;
    if (nm_exercitatio_initia(&ex, &nm.config) < 0) {
        fprintf(stderr, "erratum: nm_exercitatio_initia defecit\n");
        nm_fini(&nm);
        free(signa);
        nm_lexator_destrue(lex);
        pfr_computo_fini();
        return 1;
    }

    unsigned int semen = 42u;
    int passus_factus  = 0;
    while (passus_factus < n_passus) {
        int batch = TUTELA_PASSUS;
        if (passus_factus + batch > n_passus)
            batch = n_passus - passus_factus;

        exercita(
            &nm, &ex, signa, n_signa, batch, DEF_SEQ_LON,
            passus_factus, &semen
        );
        passus_factus += batch;

        /* tutela intermedia */
        if (nm_serva(&nm, via_bin) == 0)
            printf(
                "  [tutela: passus %d/%d -> %s]\n",
                passus_factus, n_passus, via_bin
            );
        else
            fprintf(stderr, "  admonitio: tutela defecit\n");
        fflush(stdout);
    }

    nm_exercitatio_fini(&ex);
    free(signa);
    printf("\n");

    /* --- 5. salvatio finalis (iam facta per tutelam) --- */
    printf("5. exemplar servatum: %s\n\n", via_bin);

    nm_fini(&nm);
    nm_lexator_destrue(lex);
    pfr_computo_fini();

    printf("=== exercitatio finita ===\n");
    return 0;
}
