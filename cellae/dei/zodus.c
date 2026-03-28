/*
 * zodus.c — zodus, genus phylo DEI, a lusore gubernatus, PELLE solum
 */

#include "cella.h"
#include "tabula.h"

int zodus_imperium = 0;   /* DIR_NIHIL */

static void zodus_praepara(cella_t *c)
{
    c->pondus = 50;
    deus_praepara(c);
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
    cella_initia_ops(ZODUS, "\xE2\x9A\xA1", 'Z');

    genera_ops[ZODUS].phylum   = DEI;
    genera_ops[ZODUS].praepara = zodus_praepara;
    genera_ops[ZODUS].cogito   = zodus_cogito;
}
