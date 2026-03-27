/*
 * oracula/fictus.c — provisor fictus pro probatione
 *
 * Non HTTP rogat — actiones programmatice generat delegando
 * ad genus_ops[g].fictio() cuiusque generis.
 * Usus: MUNDA_SAPIENTIA=fictus/probatio
 */

#include "provisor.h"
#include "../cella.h"
#include "../fictio.h"
#include "../json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* definitio debilis — fare non habet genera_ops, lude/curre habent */
__attribute__((weak)) genus_ops_t genera_ops[GENERA_NUMERUS];

/* lege vicinitatem ex JSON crudo array in graticulam */
static void lege_vicinitatem(const char *vic_json,
                             fictio_vicinitas_t *vic)
{
    memset(vic, '.', sizeof(vic->graticula));
    vic->latus = 0;
    vic->series = 0;
    vic->cx = 0;
    vic->cy = 0;

    if (!vic_json) return;

    const char *p = vic_json;
    int r = 0, col = 0;
    while (*p) {
        if (*p == '[') {
            if (p > vic_json && *(p - 1) == ',') { r++; col = 0; }
        } else if (*p == '"') {
            p++;
            if (*p && *p != '"') {
                if (r < FICTIO_LATUS_MAX && col < FICTIO_LATUS_MAX) {
                    vic->graticula[r][col] = *p;
                    if (*p == '@') { vic->cx = col; vic->cy = r; }
                }
                col++;
                if (col > vic->latus) vic->latus = col;
            }
            /* transili ad " clausum */
            while (*p && *p != '"') p++;
        }
        p++;
    }
    vic->series = r + 1;
}

/* quaere genus ex signo (prima littera nominis) */
static genus_t quaere_genus_ex_signo(char signum)
{
    for (int g = 0; g < GENERA_NUMERUS; g++) {
        if (genera_ops[g].signum == signum)
            return (genus_t)g;
    }
    return VACUUM;
}

/* --- provisor interfacies --- */

static int para(const char *nomen, const char *conatus,
                const char *clavis_api,
                const char *instructiones, const char *rogatum,
                char **corpus, struct crispus_slist **capita)
{
    (void)nomen; (void)conatus; (void)clavis_api;
    (void)instructiones; (void)capita;

    *corpus = strdup(rogatum);
    *capita = NULL;
    return 0;
}

static const char *const sententiae[] = {
    "Quidquid Latine dictum sit, altum videtur.",
    "Fiesta lente!",
    "Alea iacta est.",
    "Cogito, ergo sum.",
    "Dum spiro, spero.",
    "Semper und err, sub und err.",
    "Carpe diem.",
    "Par coquorum faces ad astra feret!",
    "Venti, vidi, vinci.",
    "Memento mori.",
    "Sic transit gloria mundi.",
    "Errare humanum est.",
    "Ad astra per aspera.",
};
#define SENTENTIAE_NUM (int)(sizeof(sententiae) / sizeof(sententiae[0]))

static char *extrahe(const char *json)
{
    /* proba an sit rogatum ludi (JSON cum clavibus animorum) */
    char claves[256][64];
    int nc = json_claves(json, claves, 256);

    /* si non JSON ludi, redde sententiam */
    if (nc <= 0 || !json_da_crudum(json, claves[0][0] ?
                    (char[]){claves[0][0], '\0'} : ".")) {
        /* nc <= 0: non JSON objectum, vel nc > 0 sed sine vicinitate */
        return strdup(sententiae[rand() % SENTENTIAE_NUM]);
    }

    /* verifica an prima clavis habeat vicinitatem — si non, est rogatum fare */
    {
        char via[128];
        snprintf(via, sizeof(via), "%s.vicinitas", claves[0]);
        char *vic = json_da_crudum(json, via);
        if (!vic)
            return strdup(sententiae[rand() % SENTENTIAE_NUM]);
        free(vic);
    }

    json_scriptor_t *js = json_scriptor_crea();
    if (!js) return strdup("{}");

    for (int i = 0; i < nc; i++) {
        char signum = claves[i][0];
        genus_t genus = quaere_genus_ex_signo(signum);

        char via[128];
        snprintf(via, sizeof(via), "%s.vicinitas", claves[i]);
        char *vic_json = json_da_crudum(json, via);

        if (genera_ops[genus].fictio) {
            fictio_vicinitas_t vic;
            lege_vicinitatem(vic_json, &vic);

            char actio[128];
            actio[0] = '\0';
            genera_ops[genus].fictio(claves[i], &vic,
                                     actio, sizeof(actio));
            json_scriptor_adde(js, claves[i], actio);
        } else {
            json_scriptor_adde(js, claves[i], "quiesce");
        }

        free(vic_json);
    }

    return json_scriptor_fini(js);
}

static void signa(const char *json, long *accepta, long *recondita,
                   long *emissa, long *cogitata)
{
    (void)json;
    *accepta = 0; *recondita = 0; *emissa = 0; *cogitata = 0;
}

const provisor_t provisor_fictus = {
    .nomen      = "fictus",
    .clavis_env = "",
    .finis_url  = "",
    .para       = para,
    .extrahe    = extrahe,
    .signa      = signa
};
