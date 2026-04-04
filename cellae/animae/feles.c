/*
 * feles.c — feles errans
 */

#include "cella.h"
#include "fictio.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define QUIES_LIMEN 19
#define QUIES_MODULUS 20

static const char *mentes_felium[] = {
    "esurio", "curiosa sum", "somnolenta", "quid est illud",
    "mrrr", "prrr", "ubi cibus", "fugio", "ludens",
    "vidi aliquid", "audivi sonum", "mus ubi est",
    "calida sum", "frigida sum", "sola sum",
};

static void feles_praepara(cella_t *c)
{
    c->pondus = 3;
    animus_praepara(c);
}

static actio_t feles_cogito(const struct tabula *tab, int x, int y)
{
    (void)tab;
    (void)x;
    (void)y;
    actio_t act = ACTIO_NIHIL;

    /* fortuito muta mentem */
    if (rand() % 120 == 0) {
        int n = (int)(sizeof(mentes_felium) / sizeof(mentes_felium[0]));
        snprintf(act.mens, MENS_MAX, "%s", mentes_felium[rand() % n]);
    }

    if (rand() % QUIES_MODULUS < QUIES_LIMEN)
        return act;

    act.modus    = CAPE;
    act.directio = (directio_t)(1 + rand() % 4);
    return act;
}

/* --- fictio: erra fortuito, ede cibum, fuge ursos --- */

static void feles_fictio(
    const char *nomen,
    const struct fictio_vicinitas *vic,
    char *actio, size_t mag
) {
    (void)nomen;

    /* fuge ursum vicinum */
    for (int d = SEPTENTRIO; d <= ORIENS; d++) {
        if (fictio_vicinum_est(vic, 'U', (directio_t)d)) {
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
        if (
            fictio_vicinum_est(vic, 'r', (directio_t)d) ||
            fictio_vicinum_est(vic, 'f', (directio_t)d)
        ) {
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
    cella_initia_ops(FELES, "\xF0\x9F\x90\xB1", 'F');

    genera_ops[FELES].praepara = feles_praepara;
    genera_ops[FELES].cogito   = feles_cogito;
    genera_ops[FELES].fictio   = feles_fictio;
}
