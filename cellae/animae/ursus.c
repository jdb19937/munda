/*
 * ursus.c — ursus qui saxa trahit, oraculum rogat
 */

#include "cella.h"
#include "tabula.h"
#include "cogitatio.h"
#include "fictio.h"
#include "utilia.h"

#include <stdio.h>

static praecogitata_t praecogitata;
static char *instructiones;

static void ursus_praepara(cella_t *c)
{
    c->pondus = 20;
    animus_praepara(c);
}

void ursus_praecogita(tabula_t *tab)
{
    if (!instructiones)
        instructiones = lege_instructiones(tab->munda, "animae", "ursus");
    const char *sap = tab->sapientum[URSUS][0] ?
                      tab->sapientum[URSUS] : NULL;
    cogitatio_praecogita(tab, URSUS, 1, 1,
                         1, 1, 4,
                         sap, instructiones, &praecogitata);
}

static actio_t ursus_cogito(const struct tabula *tab, int x, int y)
{
    (void)tab;
    return cogitatio_quaere(&praecogitata, x, y);
}

/* --- fictio: venare feles, ede cibum, pelle saxa --- */

static void ursus_fictio(const char *nomen,
                         const struct fictio_vicinitas *vic,
                         char *actio, size_t mag)
{
    (void)nomen;

    /* oppugna felem vicinam */
    for (int d = SEPTENTRIO; d <= ORIENS; d++) {
        if (fictio_vicinum_est(vic, 'F', (directio_t)d)) {
            snprintf(actio, mag, "oppugna %s", fictio_dir_nomen((directio_t)d));
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

    /* pelle saxum si in via ad felem */
    directio_t dir_feles = fictio_quaere_proximum(vic, 'F');
    if (dir_feles != DIR_NIHIL &&
        fictio_vicinum_est(vic, '#', dir_feles)) {
        snprintf(actio, mag, "pelle %s", fictio_dir_nomen(dir_feles));
        return;
    }

    /* move versus felem proximam */
    if (dir_feles != DIR_NIHIL) {
        snprintf(actio, mag, "move %s", fictio_dir_nomen(dir_feles));
        return;
    }

    /* explora: move fortuito */
    snprintf(actio, mag, "move %s", fictio_dir_nomen(fictio_dir_fortuita()));
}

void ursus_initia(void)
{
    cella_initia_ops(URSUS, "\xF0\x9F\x90\xBB", 'U');

    genera_ops[URSUS].phylum   = ANIMA;
    genera_ops[URSUS].praepara = ursus_praepara;
    genera_ops[URSUS].cogito   = ursus_cogito;
    genera_ops[URSUS].fictio   = ursus_fictio;
}
