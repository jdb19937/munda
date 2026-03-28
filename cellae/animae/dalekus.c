/*
 * dalekus.c — dalekus quod oraculum groupnar rogat
 */

#include "cella.h"
#include "tabula.h"
#include "cogitatio.h"
#include "fictio.h"
#include "utilia.h"

#include <stdio.h>
#include <stdlib.h>

static praecogitata_t praecogitata;
static char *instructiones;

static void dalekus_praepara(cella_t *c)
{
    c->pondus = 5;
    animus_praepara(c);
}

void dalekus_praecogita(tabula_t *tab)
{
    if (!instructiones)
        instructiones = lege_instructiones(tab->munda, "animae", "dalekus");
    const char *sap = tab->sapientum[DALEKUS][0] ?
                      tab->sapientum[DALEKUS] : NULL;
    cogitatio_praecogita_tabulam(tab, DALEKUS,
                                  sap, instructiones, &praecogitata);
}

static actio_t dalekus_cogito(const struct tabula *tab, int x, int y)
{
    (void)tab;
    return cogitatio_quaere(&praecogitata, x, y);
}

/* --- fictio: venare ursum, coopera cum sociis --- */

static void dalekus_fictio(const char *nomen,
                             const struct fictio_vicinitas *vic,
                             char *actio, size_t mag)
{
    (void)nomen;

    /* oppugna ursum vicinum */
    for (int d = SEPTENTRIO; d <= ORIENS; d++) {
        if (fictio_vicinum_est(vic, 'U', (directio_t)d)) {
            snprintf(actio, mag, "oppugna %s", fictio_dir_nomen((directio_t)d));
            return;
        }
    }

    /* move versus ursum proximum */
    directio_t dir_ursus = fictio_quaere_proximum(vic, 'U');
    if (dir_ursus != DIR_NIHIL) {
        if (rand() % 4 == 0) {
            snprintf(actio, mag, "clama ursus ad %s!",
                     fictio_dir_nomen(dir_ursus));
            return;
        }
        snprintf(actio, mag, "move %s", fictio_dir_nomen(dir_ursus));
        return;
    }

    /* cape cibum vicinum */
    for (int d = SEPTENTRIO; d <= ORIENS; d++) {
        if (fictio_vicinum_est(vic, 'r', (directio_t)d) ||
            fictio_vicinum_est(vic, 'f', (directio_t)d)) {
            snprintf(actio, mag, "cape %s", fictio_dir_nomen((directio_t)d));
            return;
        }
    }

    /* explora: move fortuito */
    snprintf(actio, mag, "move %s", fictio_dir_nomen(fictio_dir_fortuita()));
}

void dalekus_initia(void)
{
    cella_initia_ops(DALEKUS, "\xF0\x9F\xA4\x96", 'B');

    genera_ops[DALEKUS].praepara     = dalekus_praepara;
    genera_ops[DALEKUS].cogito       = dalekus_cogito;
    genera_ops[DALEKUS].fictio   = dalekus_fictio;
}
