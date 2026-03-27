/*
 * feles.c — feles errans
 */

#include "cella.h"
#include "fictio.h"
#include "utilia.h"
#include <stdlib.h>
#include <stdio.h>

#define QUIES_LIMEN 19
#define QUIES_MODULUS 20

static json_par_t pp[12];
static int pp_n;

static void feles_praepara(cella_t *c)
{
    cella_praepara(c, FELES, pp, pp_n);
    animus_praepara(c, pp, pp_n);
}

static actio_t feles_cogito(const struct tabula *tab, int x, int y)
{
    (void)tab; (void)x; (void)y;
    if (rand() % QUIES_MODULUS < QUIES_LIMEN)
        return ACTIO_NIHIL;
    directio_t dir = (directio_t)(1 + rand() % 4);
    return (actio_t){ CAPE, dir, {0}, {0} };
}

/* --- fictio: erra fortuito, ede cibum, fuge ursos --- */

static void feles_fictio(const char *nomen,
                         const struct fictio_vicinitas *vic,
                         char *actio, size_t mag)
{
    (void)nomen;

    /* fuge ursum vicinum */
    for (int d = SEPTENTRIO; d <= ORIENS; d++) {
        if (fictio_vicinum_est(vic, 'U', (directio_t)d)) {
            /* move in directionem oppositam */
            directio_t fuga;
            switch ((directio_t)d) {
            case SEPTENTRIO: fuga = MERIDIES;   break;
            case MERIDIES:   fuga = SEPTENTRIO; break;
            case OCCIDENS:   fuga = ORIENS;     break;
            case ORIENS:     fuga = OCCIDENS;   break;
            default:         fuga = fictio_dir_fortuita(); break;
            }
            snprintf(actio, mag, "move %s", fictio_dir_nomen(fuga));
            return;
        }
    }

    /* cape cibum vicinum */
    for (int d = SEPTENTRIO; d <= ORIENS; d++) {
        if (fictio_vicinum_est(vic, 'r', (directio_t)d) ||
            fictio_vicinum_est(vic, 'f', (directio_t)d)) {
            snprintf(actio, mag, "cape %s", fictio_dir_nomen((directio_t)d));
            return;
        }
    }

    /* plerumque quiesce, interdum erra */
    if (rand() % QUIES_MODULUS < QUIES_LIMEN) {
        snprintf(actio, mag, "quiesce");
        return;
    }

    snprintf(actio, mag, "move %s", fictio_dir_nomen(fictio_dir_fortuita()));
}

void feles_initia(void)
{
    pp_n = lege_parametra(__FILE__, pp, 12);
    cella_initia_ops(FELES, pp, pp_n);

    genera_ops[FELES].phylum   = ANIMA;
    genera_ops[FELES].praepara = feles_praepara;
    genera_ops[FELES].cogito   = feles_cogito;
    genera_ops[FELES].fictio   = feles_fictio;
}
