/*
 * saxum.c — lapis mobilis
 */

#include "cella.h"
#include "utilia.h"

static json_par_t pp[8];
static int pp_n;

static void saxum_praepara(cella_t *c)
{
    cella_praepara(c, SAXUM, pp, pp_n);
}

static actio_t saxum_cogito(const struct tabula *tab, int x, int y)
{
    (void)tab; (void)x; (void)y;
    return ACTIO_NIHIL;
}

void saxum_initia(void)
{
    pp_n = lege_parametra(__FILE__, pp, 8);
    cella_initia_ops(SAXUM, pp, pp_n);

    genera_ops[SAXUM].phylum   = FIXUM;
    genera_ops[SAXUM].praepara = saxum_praepara;
    genera_ops[SAXUM].cogito   = saxum_cogito;
}
