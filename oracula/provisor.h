/*
 * provisor.h — interfacies provisorum oraculi
 *
 * Quisque provisor (OpenAI, xAI, Anthropic) implementat hanc interfaciem.
 */

#ifndef PROVISOR_H
#define PROVISOR_H

#include "../crispus/crispus.h"

typedef struct provisor {
    const char *nomen;          /* "openai", "xai", "anthropic", "munda", "fictus" */
    const char *clavis_env;     /* nomen env var pro clave API */
    const char *finis_url;      /* URL finis API */

    /*
     * para rogatum HTTP.
     *   nomen        — nomen sapientis (sine provider/ et +effort)
     *   conatus      — "low"/"medium"/"high" vel "" si non habetur
     *   clavis_api   — valor clavis API
     *   instructiones — instructiones (potest esse NULL)
     *   rogatum      — rogatum usoris
     *   corpus       — *corpus allocatur (JSON body)
     *   capita       — *capita allocatur (HTTP headers)
     * reddit 0 si successum.
     */
    int (*para)(const char *nomen, const char *conatus,
                const char *clavis_api,
                const char *instructiones, const char *rogatum,
                char **corpus, struct crispus_slist **capita);

    /* extrahe textum responsi ex JSON crudo API */
    char *(*extrahe)(const char *json);

    /* extrahe numeros signorum ex JSON crudo API */
    void (*signa)(const char *json, long *accepta, long *recondita,
                  long *emissa, long *cogitata);
} provisor_t;

extern const provisor_t provisor_openai;
extern const provisor_t provisor_xai;
extern const provisor_t provisor_anthropic;
extern const provisor_t provisor_munda;
extern const provisor_t provisor_fictus;

#endif /* PROVISOR_H */
