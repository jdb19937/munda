/*
 * deus.c — functiones communes phylo DEI
 */

#include <stdio.h>
#include "cella.h"
#include "utilia.h"

void deus_praepara(cella_t *c, const json_par_t *pp, int n)
{
    c->deus.potentia = par_da_int(pp, n, "potentia", 10);
    const char *m = par_da_chordam(pp, n, "mens", "");
    if (m[0])
        snprintf(c->deus.mens, MENS_MAX, "%s", m);
}
