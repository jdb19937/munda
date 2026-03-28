/*
 * saxum.c — lapis mobilis
 */

#include "cella.h"

static void saxum_praepara(cella_t *c)
{
    c->pondus = 10;
}

static actio_t saxum_cogito(const struct tabula *tab, int x, int y)
{
    (void)tab; (void)x; (void)y;
    return ACTIO_NIHIL;
}

void saxum_initia(void)
{
    cella_initia_ops(SAXUM, "\xF0\x9F\xA7\xB1", '#');

    genera_ops[SAXUM].praepara = saxum_praepara;
    genera_ops[SAXUM].cogito   = saxum_cogito;
}
