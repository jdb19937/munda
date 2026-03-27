/*
 * murus.c — murus immobilis
 */

#include "cella.h"
#include "utilia.h"

static json_par_t pp[8];
static int pp_n;

static void murus_praepara(cella_t *c)
{
    cella_praepara(c, MURUS, pp, pp_n);
}

void murus_initia(void)
{
    pp_n = lege_parametra(__FILE__, pp, 8);
    cella_initia_ops(MURUS, pp, pp_n);

    genera_ops[MURUS].phylum   = FIXUM;
    genera_ops[MURUS].praepara = murus_praepara;
    genera_ops[MURUS].cogito   = NULL;
}
