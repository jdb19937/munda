/*
 * oculus.c — oculus, deus omnividens qui per oraculum cogitat
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

static void oculus_praepara_fn(cella_t *c)
{
    c->pondus = 40;
    deus_praepara(c);
    c->g.oculus.visus_radius = 5;
}

void oculus_praecogita(tabula_t *tab)
{
    if (!instructiones)
        instructiones = lege_instructiones(tab->munda, "dei", "oculus");
    const char *sap = tab->sapientum[OCULUS][0] ?
        tab->sapientum[OCULUS] : NULL;
    cogitatio_praecogita(
        tab, OCULUS, 1, 1,
        PRAECOGITATA_MAX, 1, 5,
        sap, instructiones, &praecogitata
    );
}

static actio_t oculus_cogito(const struct tabula *tab, int x, int y)
{
    (void)tab;
    return cogitatio_quaere(&praecogitata, x, y);
}

/* --- fictio: observa, move, loquere ad animas --- */

static void oculus_fictio(
    const char *nomen,
    const struct fictio_vicinitas *vic,
    char *actio, size_t mag
) {
    (void)nomen;

    /* loquere ad animum vicinum */
    for (int d = SEPTENTRIO; d <= ORIENS; d++) {
        if (
            fictio_vicinum_est(vic, 'F', (directio_t)d) ||
            fictio_vicinum_est(vic, 'B', (directio_t)d) ||
            fictio_vicinum_est(vic, 'U', (directio_t)d)
        ) {
            snprintf(
                actio, mag, "loquere %s cave!",
                fictio_dir_nomen((directio_t)d)
            );
            return;
        }
    }

    /* clama si ursus in vicinitate */
    directio_t dir_ursus = fictio_quaere_proximum(vic, 'U');
    if (dir_ursus != DIR_NIHIL) {
        snprintf(
            actio, mag, "clama ursus ad %s!",
            fictio_dir_nomen(dir_ursus)
        );
        return;
    }

    /* move fortuito vel quiesce */
    if (rand() % 3 == 0)
        snprintf(actio, mag, "move %s", fictio_dir_nomen(fictio_dir_fortuita()));
    else
        snprintf(actio, mag, "quiesce");
}

void oculus_initia(void)
{
    cella_initia_ops(OCULUS, "I ", 'O');

    genera_ops[OCULUS].praepara = oculus_praepara_fn;
    genera_ops[OCULUS].cogito   = oculus_cogito;
    genera_ops[OCULUS].fictio   = oculus_fictio;
}
