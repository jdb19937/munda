/*
 * cibus.c — functiones communes phylo CIBUS
 */

#include "cella.h"
#include "utilia.h"
#include "json.h"

void cibus_praepara(cella_t *c, const json_par_t *pp, int n)
{
    c->cibus.nutritio = par_da_int(pp, n, "nutritio", 1);
}
