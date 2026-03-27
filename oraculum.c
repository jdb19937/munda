/*
 * oraculum.c — dispatcher oraculi cum provisoribus multis
 *
 * Interfacies publica manet eadem. Provisor eligitur ex modello:
 *   "openai/gpt-5.4+high" → provisor_openai
 *   "xai/grok-3"          → provisor_xai
 *   "anthropic/claude-..."  → provisor_anthropic
 *   "gpt-5.4" (sine praefixo) → provisor_openai (praefinitus)
 */

#include "oraculum.h"
#include "oracula/provisor.h"
#include "utilia.h"

#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SAPIENTUM_PRAEFINITUM "fictus/default"
#define FOSSA_MAX 32

/* --- provisores --- */

static const provisor_t *provisores[8] = {
    &provisor_openai,
    &provisor_xai,
    &provisor_anthropic,
    &provisor_munda,
    &provisor_fictus,
    NULL
};

void oraculum_adde_provisorem(const provisor_t *prov)
{
    for (int i = 0; i < 7; i++) {
        if (!provisores[i]) {
            provisores[i] = prov;
            provisores[i + 1] = NULL;
            return;
        }
    }
}

static const provisor_t *quaere_provisorem(const char *nomen)
{
    if (!nomen || !*nomen)
        return &provisor_openai;
    for (int i = 0; provisores[i]; i++)
        if (strcmp(provisores[i]->nomen, nomen) == 0)
            return provisores[i];
    return &provisor_openai;
}

/* --- memoria ad responsum colligendum --- */

struct memoria {
    char  *data;
    size_t magnitudo;
};

static size_t scribe_fn(void *contenta, size_t mag, size_t nmemb, void *usor)
{
    size_t realis = mag * nmemb;
    struct memoria *mem = usor;
    char *novum = realloc(mem->data, mem->magnitudo + realis + 1);
    if (!novum) return 0;
    mem->data = novum;
    memcpy(mem->data + mem->magnitudo, contenta, realis);
    mem->magnitudo += realis;
    mem->data[mem->magnitudo] = '\0';
    return realis;
}

/* --- resolve sapientum et provisorem --- */

static char sapientum_currens[128];
static const provisor_t *provisor_currentis;

/* resolve "default" in nomen sapientis frontieris */
static const char *resolve_default(const char *provisor, const char *nomen)
{
    if (strcmp(nomen, "default") != 0)
        return nomen;
    if (!*provisor || strcmp(provisor, "openai") == 0)
        return "gpt-5.4";
    if (strcmp(provisor, "anthropic") == 0)
        return "claude-sonnet-4-6";
    if (strcmp(provisor, "xai") == 0)
        return "grok-4.20";
    return nomen;
}

static const provisor_t *resolve(const char *sapientum,
                                 char *nomen, size_t nmag,
                                 char *conatus, size_t cmag)
{
    const char *spec = sapientum;
    if (!spec || !*spec) spec = getenv("MUNDA_SAPIENTIA");
    if (!spec || !*spec) spec = SAPIENTUM_PRAEFINITUM;

    char provisor[64];
    lege_sapientum(spec, provisor, sizeof(provisor),
                   nomen, nmag, conatus, cmag);

    const provisor_t *prov = quaere_provisorem(provisor);

    /* resolve "default" ad nomen frontieris */
    const char *resolutum = resolve_default(provisor, nomen);
    if (resolutum != nomen)
        snprintf(nomen, nmag, "%s", resolutum);

    /* serva sapientum plenum "provisor/nomen" */
    snprintf(sapientum_currens, sizeof(sapientum_currens),
             "%s/%s", prov->nomen, nomen);
    provisor_currentis = prov;

    return prov;
}

/* --- para CURL cum provisore --- */

static int para_curl(CURL *curl, const char *sapientum,
                     const char *instructiones, const char *rogatum,
                     char **corpus_out, struct curl_slist **capita_out,
                     struct memoria *mem)
{
    char nomen[128], conatus[32];
    const provisor_t *prov = resolve(sapientum,
                                     nomen, sizeof(nomen),
                                     conatus, sizeof(conatus));

    const char *clavis = getenv(prov->clavis_env);
    if (!clavis || !*clavis) return -1;

    char *corpus = NULL;
    struct curl_slist *capita = NULL;

    if (prov->para(nomen, conatus, clavis,
                   instructiones, rogatum,
                   &corpus, &capita) < 0)
        return -1;

    mem->data = NULL;
    mem->magnitudo = 0;

    curl_easy_setopt(curl, CURLOPT_URL, prov->finis_url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, corpus);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, capita);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, scribe_fn);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, mem);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

    *corpus_out = corpus;
    *capita_out = capita;
    return 0;
}

/* --- fossae asynchronae --- */

struct fossa {
    enum fossa_actum actum;
    CURL *curl;
    struct curl_slist *capita;
    char *corpus;
    struct memoria mem;
    char *responsum;
    int exitus;
    const provisor_t *provisor;
    char sapientum[128];    /* sapientum plenum pro attributione */
};

static CURLM *multi;
static struct fossa fossae[FOSSA_MAX];

/* numeri per sapientum (clavis = "sapientum:metrica") */
static lexicon_t *numeri_lex;

static void fossa_fini_transferum(struct fossa *f, CURLcode rc)
{
    const provisor_t *prov = f->provisor ? f->provisor : &provisor_openai;

    if (rc != CURLE_OK) {
        char err[256];
        snprintf(err, sizeof(err), "curl error: %s",
                 curl_easy_strerror(rc));
        f->responsum = strdup(err);
        f->exitus = -1;
    } else {
        long codex;
        curl_easy_getinfo(f->curl, CURLINFO_RESPONSE_CODE, &codex);
        if (!f->mem.data) {
            f->responsum = strdup("responsum vacuum");
            f->exitus = -1;
        } else {
            f->responsum = prov->extrahe(f->mem.data);
            f->exitus = (codex >= 200 && codex < 300 && f->responsum)
                        ? 0 : -1;
        }
    }

    const char *nomen = f->sapientum[0] ? f->sapientum : "ignotum";

    if (f->exitus == 0)
        lexicon_adde_compositam(numeri_lex, nomen, "successae", 1);
    else
        lexicon_adde_compositam(numeri_lex, nomen, "errores", 1);

    if (f->mem.data && prov->signa) {
        long accepta = 0, recondita = 0, emissa = 0, cogitata = 0;
        prov->signa(f->mem.data, &accepta, &recondita, &emissa, &cogitata);
        lexicon_adde_compositam(numeri_lex, nomen, "accepta", accepta);
        lexicon_adde_compositam(numeri_lex, nomen, "recondita", recondita);
        lexicon_adde_compositam(numeri_lex, nomen, "emissa", emissa);
        lexicon_adde_compositam(numeri_lex, nomen, "cogitata", cogitata);
    }

    curl_multi_remove_handle(multi, f->curl);
    curl_easy_cleanup(f->curl);
    curl_slist_free_all(f->capita);
    free(f->corpus);
    free(f->mem.data);
    f->curl = NULL;
    f->capita = NULL;
    f->corpus = NULL;
    f->mem.data = NULL;
    f->mem.magnitudo = 0;
    f->actum = FOSSA_PERFECTA;
}

/* --- interfacies publica --- */

int oraculum_initia(void)
{
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK)
        return -1;
    multi = curl_multi_init();
    if (!multi) return -1;
    memset(fossae, 0, sizeof(fossae));
    numeri_lex = lexicon_crea();
    return 0;
}

void oraculum_fini(void)
{
    for (int i = 0; i < FOSSA_MAX; i++) {
        if (fossae[i].actum == FOSSA_VOLANS && fossae[i].curl) {
            curl_multi_remove_handle(multi, fossae[i].curl);
            curl_easy_cleanup(fossae[i].curl);
            curl_slist_free_all(fossae[i].capita);
            free(fossae[i].corpus);
            free(fossae[i].mem.data);
        }
        free(fossae[i].responsum);
        fossae[i].actum = FOSSA_LIBERA;
    }
    if (multi) curl_multi_cleanup(multi);
    lexicon_libera(numeri_lex);
    numeri_lex = NULL;
    curl_global_cleanup();
}

/* --- synchrona --- */

int oraculum_roga(const char *sapientum, const char *instructiones,
                  const char *rogatum, char **responsum)
{
    *responsum = NULL;

    /* resolve provisorem */
    char nomen[128], conatus[32];
    const provisor_t *prov = resolve(sapientum,
                                     nomen, sizeof(nomen),
                                     conatus, sizeof(conatus));

    /* provisores locales: genera responsum statim, sine curl */
    if (strcmp(prov->nomen, "munda") == 0 ||
        strcmp(prov->nomen, "fictus") == 0) {
        *responsum = prov->extrahe(rogatum);
        return *responsum ? 0 : -1;
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
        *responsum = strdup("curl_easy_init defecit");
        return -1;
    }

    char *corpus = NULL;
    struct curl_slist *capita = NULL;
    struct memoria mem = { NULL, 0 };

    if (para_curl(curl, sapientum, instructiones, rogatum,
                  &corpus, &capita, &mem) < 0) {
        curl_easy_cleanup(curl);
        *responsum = strdup("clavis API deest vel para defecit");
        return -1;
    }

    CURLcode rc = curl_easy_perform(curl);
    int exitus = -1;

    if (rc != CURLE_OK) {
        char err[256];
        snprintf(err, sizeof(err), "curl error: %s",
                 curl_easy_strerror(rc));
        *responsum = strdup(err);
    } else {
        long codex;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &codex);
        if (!mem.data) {
            *responsum = strdup("responsum vacuum");
        } else {
            *responsum = prov->extrahe(mem.data);
            if (codex >= 200 && codex < 300 && *responsum)
                exitus = 0;
        }
    }

    free(mem.data);
    free(corpus);
    curl_easy_cleanup(curl);
    curl_slist_free_all(capita);
    return exitus;
}

/* --- asynchrona --- */

int oraculum_mitte(const char *sapientum, const char *instructiones,
                   const char *rogatum)
{
    int fi = -1;
    for (int i = 0; i < FOSSA_MAX; i++) {
        if (fossae[i].actum == FOSSA_LIBERA) { fi = i; break; }
    }
    if (fi < 0) return -1;

    /* resolve provisorem ut sciamus an munda sit */
    char nomen[128], conatus[32];
    const provisor_t *prov = resolve(sapientum,
                                     nomen, sizeof(nomen),
                                     conatus, sizeof(conatus));

    struct fossa *f = &fossae[fi];
    memset(f, 0, sizeof(*f));
    f->provisor = prov;
    snprintf(f->sapientum, sizeof(f->sapientum), "%s", sapientum_currens);
    lexicon_adde_compositam(numeri_lex, f->sapientum, "missae", 1);

    /* provisores locales: genera responsum statim, sine curl */
    if (strcmp(prov->nomen, "munda") == 0 ||
        strcmp(prov->nomen, "fictus") == 0) {
        f->responsum = prov->extrahe(rogatum);
        f->exitus = 0;
        f->actum = FOSSA_PERFECTA;
        lexicon_adde_compositam(numeri_lex, f->sapientum, "successae", 1);
        return fi;
    }

    CURL *curl = curl_easy_init();
    if (!curl) return -1;

    if (para_curl(curl, sapientum, instructiones, rogatum,
                  &f->corpus, &f->capita, &f->mem) < 0) {
        curl_easy_cleanup(curl);
        return -1;
    }

    f->actum = FOSSA_VOLANS;
    f->curl = curl;
    curl_multi_add_handle(multi, curl);
    return fi;
}

void oraculum_processus(void)
{
    if (!multi) return;

    int running;
    curl_multi_perform(multi, &running);

    CURLMsg *msg;
    int remaining;
    while ((msg = curl_multi_info_read(multi, &remaining))) {
        if (msg->msg != CURLMSG_DONE)
            continue;

        CURL *curl = msg->easy_handle;
        for (int i = 0; i < FOSSA_MAX; i++) {
            if (fossae[i].actum == FOSSA_VOLANS && fossae[i].curl == curl) {
                fossa_fini_transferum(&fossae[i], msg->data.result);
                break;
            }
        }
    }
}

int oraculum_status(int fi, char **responsum)
{
    *responsum = NULL;
    if (fi < 0 || fi >= FOSSA_MAX) return ORACULUM_ERRATUM;

    struct fossa *f = &fossae[fi];
    if (f->actum == FOSSA_VOLANS) return ORACULUM_PENDENS;
    if (f->actum == FOSSA_PERFECTA) {
        *responsum = f->responsum;
        f->responsum = NULL;
        return (f->exitus == 0) ? ORACULUM_PARATUM : ORACULUM_ERRATUM;
    }
    return ORACULUM_ERRATUM;
}

void oraculum_dimitte(int fi)
{
    if (fi < 0 || fi >= FOSSA_MAX) return;
    free(fossae[fi].responsum);
    fossae[fi].responsum = NULL;
    fossae[fi].actum = FOSSA_LIBERA;
}

void oraculum_numeri(oraculum_numeri_t *num)
{
    num->pendentes = 0;
    num->paratae = 0;
    num->liberae = 0;
    for (int i = 0; i < FOSSA_MAX; i++) {
        switch (fossae[i].actum) {
        case FOSSA_LIBERA:   num->liberae++;   break;
        case FOSSA_VOLANS:   num->pendentes++; break;
        case FOSSA_PERFECTA: num->paratae++;   break;
        }
    }
    /* summas computa ex lexico iterando per genera */
    num->summa_missae = 0;
    num->summa_successae = 0;
    num->summa_errores = 0;
    num->summa_signa_accepta = 0;
    num->summa_signa_recondita = 0;
    num->summa_signa_emissa = 0;
    num->summa_signa_cogitata = 0;

    char genera[16][64];
    int ngen = lexicon_genera(numeri_lex, genera, 16);
    for (int i = 0; i < ngen; i++) {
        num->summa_missae    += lexicon_da_compositam(numeri_lex, genera[i], "missae");
        num->summa_successae += lexicon_da_compositam(numeri_lex, genera[i], "successae");
        num->summa_errores   += lexicon_da_compositam(numeri_lex, genera[i], "errores");
        num->summa_signa_accepta   += lexicon_da_compositam(numeri_lex, genera[i], "accepta");
        num->summa_signa_recondita += lexicon_da_compositam(numeri_lex, genera[i], "recondita");
        num->summa_signa_emissa    += lexicon_da_compositam(numeri_lex, genera[i], "emissa");
        num->summa_signa_cogitata  += lexicon_da_compositam(numeri_lex, genera[i], "cogitata");
    }
}

const char *oraculum_sapientum(void)
{
    return sapientum_currens[0] ? sapientum_currens : SAPIENTUM_PRAEFINITUM;
}

int oraculum_numeri_per_sapientum(oraculum_numeri_modelli_t *tabulatum, int maximus)
{
    if (!numeri_lex) return 0;
    char genera[16][64];
    int ngen = lexicon_genera(numeri_lex, genera, 16);
    if (ngen > maximus) ngen = maximus;
    for (int i = 0; i < ngen; i++) {
        snprintf(tabulatum[i].sapientum, sizeof(tabulatum[i].sapientum),
                 "%s", genera[i]);
        tabulatum[i].volantes = 0;
        tabulatum[i].paratae  = 0;
        tabulatum[i].missae      = lexicon_da_compositam(numeri_lex, genera[i], "missae");
        tabulatum[i].successae   = lexicon_da_compositam(numeri_lex, genera[i], "successae");
        tabulatum[i].errores     = lexicon_da_compositam(numeri_lex, genera[i], "errores");
        tabulatum[i].signa_accepta   = lexicon_da_compositam(numeri_lex, genera[i], "accepta");
        tabulatum[i].signa_recondita = lexicon_da_compositam(numeri_lex, genera[i], "recondita");
        tabulatum[i].signa_emissa    = lexicon_da_compositam(numeri_lex, genera[i], "emissa");
        tabulatum[i].signa_cogitata  = lexicon_da_compositam(numeri_lex, genera[i], "cogitata");
    }
    /* computa volantes et paratae ex fossis vivis */
    for (int i = 0; i < FOSSA_MAX; i++) {
        if (fossae[i].actum == FOSSA_LIBERA) continue;
        const char *nomen = fossae[i].sapientum;
        if (!nomen[0]) continue;
        for (int j = 0; j < ngen; j++) {
            if (strcmp(tabulatum[j].sapientum, nomen) == 0) {
                if (fossae[i].actum == FOSSA_VOLANS)   tabulatum[j].volantes++;
                if (fossae[i].actum == FOSSA_PERFECTA) tabulatum[j].paratae++;
                break;
            }
        }
    }
    return ngen;
}
