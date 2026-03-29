/*
 * cibus.c — functiones communes phylo CIBUS
 */

#include "cella.h"
#include "utilia.h"
#include "ison.h"

void cibus_praepara(cella_t *c, const ison_par_t *pp, int n)
{
    c->p.cibus.nutritio = par_da_int(pp, n, "nutritio", 1);
}
