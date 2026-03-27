/*
 * oracula/munda.c — provisor programmatus (quasicogitatio)
 *
 * Non HTTP rogat — responsa programmatice generat ex vicinitate.
 * Usus: MUNDA_SAPIENTIA=munda/instinctus
 */

#include "provisor.h"
#include "../json.h"
#include "../cellula.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* directiones */
static const char *dir_nomina[] = {
    NULL, "septentrio", "meridies", "occidens", "oriens"
};

/* quaere signum in vicinitate (JSON array duplex) circa centrum.
 * reddit directionem ad proximum, vel DIR_NIHIL si non invenitur. */
static directio_t quaere_proximum(const char *vicinitas, char signum)
{
    /* lege vicinitatem ad 2D indicem signorum */
    int latus = 0;
    char graticula[32][32];
    memset(graticula, '.', sizeof(graticula));

    const char *p = vicinitas;
    int r = 0, col = 0;
    while (*p) {
        if (*p == '[') {
            if (p > vicinitas && *(p - 1) == ',') { r++; col = 0; }
        } else if (*p == '"') {
            p++;
            if (*p && *p != '"') {
                graticula[r][col] = *p;
                col++;
                if (col > latus) latus = col;
            }
        }
        p++;
    }
    int series = r + 1;
    if (latus == 0 || series == 0) return DIR_NIHIL;

    int cx = latus / 2;
    int cy = series / 2;

    /* quaere proximum signum */
    int optima_dist = 9999;
    int optima_dx = 0, optima_dy = 0;

    for (int ry = 0; ry < series; ry++) {
        for (int rx = 0; rx < latus; rx++) {
            if (graticula[ry][rx] == signum) {
                int dx = rx - cx;
                int dy = ry - cy;
                int dist = abs(dx) + abs(dy);
                if (dist > 0 && dist < optima_dist) {
                    optima_dist = dist;
                    optima_dx = dx;
                    optima_dy = dy;
                }
            }
        }
    }

    if (optima_dist == 9999) return DIR_NIHIL;

    /* elige directionem cardinalem dominantem */
    if (abs(optima_dy) >= abs(optima_dx))
        return (optima_dy < 0) ? SEPTENTRIO : MERIDIES;
    else
        return (optima_dx < 0) ? OCCIDENS : ORIENS;
}

/* an signum sit vicinum (distantia 1) in directione data */
static int vicinum_est(const char *vicinitas, char signum, directio_t dir)
{
    char graticula[32][32];
    memset(graticula, '.', sizeof(graticula));

    int latus = 0, series = 0;
    const char *p = vicinitas;
    int r = 0, col = 0;
    while (*p) {
        if (*p == '[') {
            if (p > vicinitas && *(p - 1) == ',') { r++; col = 0; }
        } else if (*p == '"') {
            p++;
            if (*p && *p != '"') {
                graticula[r][col] = *p;
                col++;
                if (col > latus) latus = col;
            }
        }
        p++;
    }
    series = r + 1;
    if (latus == 0) return 0;

    int cx = latus / 2;
    int cy = series / 2;
    int tx = cx, ty = cy;

    switch (dir) {
    case SEPTENTRIO: ty--; break;
    case MERIDIES:   ty++; break;
    case OCCIDENS:   tx--; break;
    case ORIENS:     tx++; break;
    default: return 0;
    }

    if (tx < 0 || tx >= latus || ty < 0 || ty >= series) return 0;
    return graticula[ty][tx] == signum;
}

/* genera responsum pro uno ente in rogatu JSON */
static void genera_actionem(const char *nomen, const char *valor,
                             json_scriptor_t *js)
{
    /* extrahe vicinitas ex valor JSON */
    char *vicinitas = json_da_chordam(valor, "vicinitas");
    if (!vicinitas) {
        json_scriptor_adde(js, nomen, "quiesce");
        return;
    }

    /* determina genus ex praefixo nominis */
    char c = nomen[0];
    char actio[128];

    if (c == 'U') {
        /* ursus: quaere felem, oppugna si vicina, aliter move versus eam */
        for (int d = SEPTENTRIO; d <= ORIENS; d++) {
            if (vicinum_est(vicinitas, 'F', (directio_t)d)) {
                snprintf(actio, sizeof(actio), "oppugna %s", dir_nomina[d]);
                json_scriptor_adde(js, nomen, actio);
                free(vicinitas);
                return;
            }
        }
        /* cibum quoque cape si vicinus */
        for (int d = SEPTENTRIO; d <= ORIENS; d++) {
            if (vicinum_est(vicinitas, 'r', (directio_t)d) ||
                vicinum_est(vicinitas, 'f', (directio_t)d)) {
                snprintf(actio, sizeof(actio), "cape %s", dir_nomina[d]);
                json_scriptor_adde(js, nomen, actio);
                free(vicinitas);
                return;
            }
        }
        /* move versus felem proximam */
        directio_t dir = quaere_proximum(vicinitas, 'F');
        if (dir != DIR_NIHIL) {
            snprintf(actio, sizeof(actio), "move %s", dir_nomina[dir]);
            json_scriptor_adde(js, nomen, actio);
        } else {
            /* explora: move in directione fortuita */
            directio_t rd = (directio_t)(1 + rand() % 4);
            snprintf(actio, sizeof(actio), "move %s", dir_nomina[rd]);
            json_scriptor_adde(js, nomen, actio);
        }
    } else if (c == 'A') {
        /* dalekus: move fortuito, interdum loquere */
        directio_t rd = (directio_t)(1 + rand() % 4);
        if (rand() % 5 == 0) {
            snprintf(actio, sizeof(actio), "clama salve!");
        } else {
            snprintf(actio, sizeof(actio), "move %s", dir_nomina[rd]);
        }
        json_scriptor_adde(js, nomen, actio);
    } else {
        /* praefinitum: move fortuito */
        directio_t rd = (directio_t)(1 + rand() % 4);
        snprintf(actio, sizeof(actio), "move %s", dir_nomina[rd]);
        json_scriptor_adde(js, nomen, actio);
    }

    free(vicinitas);
}

/* --- provisor interfacies --- */

static int para(const char *nomen, const char *conatus,
                const char *clavis_api,
                const char *instructiones, const char *rogatum,
                char **corpus, struct crispus_slist **capita)
{
    (void)nomen; (void)conatus; (void)clavis_api;
    (void)instructiones; (void)capita;

    /* corpus = rogatum ipsum — extrahe postea in extrahe() */
    *corpus = strdup(rogatum);
    *capita = NULL;
    return 0;
}

static char *extrahe(const char *json)
{
    /* json est rogatum originale — genera responsum programmatice */
    json_par_t pares[256];
    int np = json_lege(json, pares, 256);
    if (np <= 0) return strdup("{\"error\": \"lectio\"}");

    json_scriptor_t *js = json_scriptor_crea();
    if (!js) return strdup("{}");

    for (int i = 0; i < np; i++) {
        genera_actionem(pares[i].clavis, pares[i].valor, js);
    }

    return json_scriptor_fini(js);
}

static void signa(const char *json, long *accepta, long *recondita,
                   long *emissa, long *cogitata)
{
    (void)json;
    *accepta = 0; *recondita = 0; *emissa = 0; *cogitata = 0;
}

const provisor_t provisor_munda = {
    .nomen      = "munda",
    .clavis_env = "",
    .finis_url  = "",
    .para       = para,
    .extrahe    = extrahe,
    .signa      = signa
};
