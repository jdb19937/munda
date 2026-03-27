/*
 * vacuum.c — cella vacua
 */

#include "cella.h"
#include "utilia.h"

static json_par_t pp[8];
static int pp_n;

static actio_t vacuum_cogito(const struct tabula *tab, int x, int y)
{
    (void)tab; (void)x; (void)y;
    return ACTIO_NIHIL;
}

void vacuum_initia(void)
{
    pp_n = lege_parametra(__FILE__, pp, 8);
    cella_initia_ops(VACUUM, pp, pp_n);

    genera_ops[VACUUM].phylum   = FIXUM;
    genera_ops[VACUUM].cogito   = vacuum_cogito;
}
