/*
 * corvus.c — corvus, avis callida et rapax
 *
 * Corvus est avis levis, rapida, et callida. Cibum avide quaerit,
 * ursos fugit, interdum clamat "CRAW!" ad socios.
 * Per oraculum LLM cogitat.
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

static void corvus_praepara(cella_t *c)
{
    c->pondus = 2;
    animus_praepara(c);
}

void corvus_praecogita(tabula_t *tab)
{
    if (!instructiones)
        instructiones = lege_instructiones(tab->munda, "animae", "corvus");
    const char *sap = tab->sapientum[CORVUS][0] ?
                      tab->sapientum[CORVUS] : NULL;
    cogitatio_praecogita(tab, CORVUS, 1, 1,
                         1, 1, 5,
                         sap, instructiones, &praecogitata);
}

static actio_t corvus_cogito(const struct tabula *tab, int x, int y)
{
    (void)tab;
    return cogitatio_quaere(&praecogitata, x, y);
}

/* --- fictio: cape cibum, fuge ursos, clama interdum --- */

static void corvus_fictio(const char *nomen,
                          const struct fictio_vicinitas *vic,
                          char *actio, size_t mag)
{
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

    /* cape cibum vicinum — avide */
    for (int d = SEPTENTRIO; d <= ORIENS; d++) {
        if (fictio_vicinum_est(vic, 'r', (directio_t)d) ||
            fictio_vicinum_est(vic, 'f', (directio_t)d)) {
            snprintf(actio, mag, "cape %s", fictio_dir_nomen((directio_t)d));
            return;
        }
    }

    /* move versus cibum proximum */
    directio_t dir_cibus = fictio_quaere_proximum(vic, 'r');
    if (dir_cibus == DIR_NIHIL)
        dir_cibus = fictio_quaere_proximum(vic, 'f');
    if (dir_cibus != DIR_NIHIL) {
        snprintf(actio, mag, "move %s", fictio_dir_nomen(dir_cibus));
        return;
    }

    /* interdum clama */
    if (rand() % 8 == 0) {
        snprintf(actio, mag, "clama CRAW!");
        return;
    }

    /* semper move — corvus raro quiescit */
    snprintf(actio, mag, "move %s", fictio_dir_nomen(fictio_dir_fortuita()));
}

void corvus_initia(void)
{
    cella_initia_ops(CORVUS, "\xF0\x9F\x90\xA6", 'C');

    genera_ops[CORVUS].phylum   = ANIMA;
    genera_ops[CORVUS].praepara = corvus_praepara;
    genera_ops[CORVUS].cogito   = corvus_cogito;
    genera_ops[CORVUS].fictio   = corvus_fictio;
}
