/*
 * rapum.c — rapum edibile
 */

#include "cella.h"
#include "utilia.h"

static json_par_t pp[8];
static int pp_n;

static void rapum_praepara(cella_t *c)
{
    cella_praepara(c, RAPUM, pp, pp_n);
}

void rapum_initia(void)
{
    pp_n = lege_parametra(__FILE__, pp, 8);
    cella_initia_ops(RAPUM, pp, pp_n);

    genera_ops[RAPUM].phylum   = CIBUS;
    genera_ops[RAPUM].praepara = rapum_praepara;
    genera_ops[RAPUM].cogito   = NULL;
}
