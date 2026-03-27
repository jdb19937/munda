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

static json_par_t pp[12];
static int pp_n;

static void dalekus_praepara(cella_t *c)
{
    cella_praepara(c, DALEKUS, pp, pp_n);
    animus_praepara(c, pp, pp_n);
}

void dalekus_praecogita(tabula_t *tab)
{
    if (!instructiones)
        instructiones = lege_instructiones(__FILE__);
    cogitatio_praecogita_tabulam(tab, DALEKUS,
                                  NULL, instructiones, &praecogitata);
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
        /* interdum clama positionem ursi */
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
    pp_n = lege_parametra(__FILE__, pp, 12);
    cella_initia_ops(DALEKUS, pp, pp_n);

    genera_ops[DALEKUS].phylum   = ANIMA;
    genera_ops[DALEKUS].praepara = dalekus_praepara;
    genera_ops[DALEKUS].cogito   = dalekus_cogito;
    genera_ops[DALEKUS].fictio   = dalekus_fictio;
}
