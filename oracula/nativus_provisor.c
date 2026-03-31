/*
 * nativus_provisor.c — provisor_t "nativus" pro oraculo
 *
 * Implementat interfaciem provisoris pro inferentiis localibus
 * ex exemplari transformer nativo. Nullum HTTP, nullum API.
 *
 * Conventio plicarum:
 *   exemplar:   ./<nomen>.bin
 *   lexator:    ./<nomen>.lex
 */

#include "nativus.h"
#include "nativus_lexator.h"
#include "oracula/provisor.h"
#include "utilia.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ================================================================
 * status globalis
 * ================================================================ */

static char nativus_nomen_currens[256] = "praefinitus";

static nm_t         nativus_exemplar;
static nm_lexator_t *nativus_lexator_currens = NULL;
static char          nativus_nomen_exemplaris[256] = "";
static int           nativus_initiatus = 0;

/* ================================================================
 * initium exemplaris
 * ================================================================ */

void nativus_pone_nomen(const char *nomen)
{
    snprintf(nativus_nomen_currens, sizeof(nativus_nomen_currens),
             "%s", nomen);
}

static int nativus_lege_exemplar(const char *nomen)
{
    if (strcmp(nomen, nativus_nomen_exemplaris) == 0 && nativus_initiatus)
        return 0; /* iam initiatus */

    if (nativus_initiatus) {
        nm_fini(&nativus_exemplar);
        if (nativus_lexator_currens) {
            nm_lexator_destrue(nativus_lexator_currens);
            nativus_lexator_currens = NULL;
        }
        nativus_initiatus = 0;
    }

    char via_bin[512], via_lex[512];
    snprintf(via_bin, sizeof(via_bin), "%s.bin", nomen);
    snprintf(via_lex, sizeof(via_lex), "%s.lex", nomen);

    if (nm_lege(&nativus_exemplar, via_bin) < 0) {
        fprintf(stderr, "nativus: non potui legere '%s'\n", via_bin);
        return -1;
    }

    nativus_lexator_currens = nm_lexator_crea(NM_LEX_VOCAB_PRAEFIN);
    if (!nativus_lexator_currens ||
        nm_lexator_lege(nativus_lexator_currens, via_lex) < 0) {
        fprintf(stderr, "nativus: non potui legere '%s'\n", via_lex);
        nm_fini(&nativus_exemplar);
        return -1;
    }

    snprintf(nativus_nomen_exemplaris, sizeof(nativus_nomen_exemplaris),
             "%s", nomen);
    nativus_initiatus = 1;
    return 0;
}

/* ================================================================
 * para — non adhibetur pro provisoribus localibus (stub)
 * ================================================================ */

static int nativus_para(const char *nomen, const char *conatus,
                        const char *clavis_api,
                        const char *instructiones, const char *rogatum,
                        char **corpus, struct crispus_slist **capita)
{
    (void)nomen; (void)conatus; (void)clavis_api;
    (void)instructiones;
    *corpus = strdup(rogatum ? rogatum : "");
    *capita = NULL;
    return *corpus ? 0 : -1;
}

/* ================================================================
 * extrahe — inferentiae transformer nativus
 * ================================================================ */

#define NATIVUS_SIGNA_MAX      256
#define NATIVUS_GENERA_MAX     256
#define NATIVUS_CALOR_PRAEFIN  0.8f

static char *nativus_extrahe(const char *rogatum)
{
    if (nativus_lege_exemplar(nativus_nomen_currens) < 0)
        return strdup("nativus: exemplar legere non potui");

    nm_t         *nm  = &nativus_exemplar;
    nm_lexator_t *lex = nativus_lexator_currens;
    nm_config_t  *c   = &nm->config;
    int V = c->vocab_magnitudo;

    /* tokeniza rogatum */
    int n_rogati = (int)strlen(rogatum) + 4;
    int *signa_rogati = malloc((size_t)n_rogati * sizeof(int));
    if (!signa_rogati) return strdup("memoria deest");
    int n_sig_rogati = 0;
    nm_lexator_disseca(lex, rogatum, 1, 0, signa_rogati, &n_sig_rogati);

    nm_status_restitue(nm);

    /* processus signorum rogati */
    float *logitae = NULL;
    for (int t = 0; t < n_sig_rogati && t < c->longitudo_max - 1; t++)
        logitae = nm_cursus(nm, signa_rogati[t], t);

    int positio = n_sig_rogati < c->longitudo_max ? n_sig_rogati : c->longitudo_max - 1;
    free(signa_rogati);

    /* generatio */
    int signa_gen[NATIVUS_SIGNA_MAX];
    int n_gen = 0;
    unsigned int semen = (unsigned int)time(NULL);

    for (int pas = 0; pas < NATIVUS_GENERA_MAX && positio < c->longitudo_max; pas++) {
        if (!logitae) break;

        /* sampling cu temperatura */
        float *prob = malloc((size_t)V * sizeof(float));
        if (!prob) break;
        memcpy(prob, logitae, (size_t)V * sizeof(float));

        /* applica temperatura */
        float calor = NATIVUS_CALOR_PRAEFIN;
        for (int i = 0; i < V; i++) prob[i] /= calor;

        /* softmax */
        float mx = prob[0];
        for (int i = 1; i < V; i++) if (prob[i] > mx) mx = prob[i];
        float s = 0.0f;
        for (int i = 0; i < V; i++) { prob[i] = expf(prob[i] - mx); s += prob[i]; }
        for (int i = 0; i < V; i++) prob[i] /= s;

        /* campionamento multinomiale */
        semen = semen * 1664525u + 1013904223u;
        float r = ((float)(semen & 0x7fffffff)) / (float)0x7fffffff;
        float cdf = 0.0f;
        int prox = V - 1;
        for (int i = 0; i < V; i++) {
            cdf += prob[i];
            if (r < cdf) { prox = i; break; }
        }
        free(prob);

        if (prox == 2) break; /* signum finis */
        signa_gen[n_gen++] = prox;
        logitae = nm_cursus(nm, prox, positio++);
    }

    /* decodifica — concatena omnia signa in unum tampon */
    size_t raw_mag = (size_t)n_gen * NM_LEX_VOCABULUM_MAX + 1;
    char *raw = malloc(raw_mag);
    if (!raw) return strdup("memoria deest");
    size_t cursor = 0;
    for (int i = 0; i < n_gen; i++) {
        const char *t = nm_lexator_redde(lex, signa_gen[i]);
        size_t tl = strlen(t);
        if (cursor + tl < raw_mag) {
            memcpy(raw + cursor, t, tl);
            cursor += tl;
        }
    }
    raw[cursor] = '\0';

    /* pura UTF-8: omitte characteres invalidos et non-impressibiles */
    char *exitus = malloc(raw_mag);
    if (!exitus) { free(raw); return strdup("memoria deest"); }
    utf8_mundus(exitus, raw_mag, raw);
    free(raw);
    return exitus;
}

/* ================================================================
 * signa — numeri signorum ex generatione
 * ================================================================ */

static void nativus_signa(const char *ison, long *accepta, long *recondita,
                          long *emissa, long *cogitata)
{
    (void)ison;
    *accepta  = 0;
    *recondita = 0;
    *emissa   = 0;
    *cogitata = 0;
}

/* ================================================================
 * provisor_t
 * ================================================================ */

const provisor_t provisor_nativus = {
    "nativus",
    "",           /* clavis_env: non opus est */
    "",           /* finis_url: localis */
    nativus_para,
    nativus_extrahe,
    nativus_signa
};
