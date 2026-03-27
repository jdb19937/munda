/*
 * zodus.c — zodus, genus phylo DEI, a lusore gubernatus, PELLE solum
 */

#include "cella.h"
#include "tabula.h"
#include "utilia.h"

int zodus_imperium = 0;   /* DIR_NIHIL */

static json_par_t pp[12];
static int pp_n;

static void zodus_praepara(cella_t *c)
{
    cella_praepara(c, ZODUS, pp, pp_n);
    deus_praepara(c, pp, pp_n);
}

static actio_t zodus_cogito(const struct tabula *tab, int x, int y)
{
    (void)tab; (void)x; (void)y;

    directio_t dir = (directio_t)zodus_imperium;
    zodus_imperium = 0;

    if (dir == DIR_NIHIL)
        return ACTIO_NIHIL;

    actio_t act = ACTIO_NIHIL;
    act.modus = PELLE;
    act.directio = dir;
    return act;
}

void zodus_initia(void)
{
    pp_n = lege_parametra(__FILE__, pp, 12);
    cella_initia_ops(ZODUS, pp, pp_n);

    genera_ops[ZODUS].phylum   = DEI;
    genera_ops[ZODUS].praepara = zodus_praepara;
    genera_ops[ZODUS].cogito   = zodus_cogito;
}
