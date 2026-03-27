/*
 * fungus.c — fungus edibilis
 */

#include "cella.h"
#include "utilia.h"

static json_par_t pp[8];
static int pp_n;

static void fungus_praepara(cella_t *c)
{
    cella_praepara(c, FUNGUS, pp, pp_n);
}

void fungus_initia(void)
{
    pp_n = lege_parametra(__FILE__, pp, 8);
    cella_initia_ops(FUNGUS, pp, pp_n);

    genera_ops[FUNGUS].phylum   = CIBUS;
    genera_ops[FUNGUS].praepara = fungus_praepara;
    genera_ops[FUNGUS].cogito   = NULL;
}
