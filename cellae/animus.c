/*
 * animus.c — functiones communes phylorum ANIMA et DEI
 */

#include <stdio.h>
#include "cella.h"
#include "utilia.h"

void animus_praepara(cella_t *c, const json_par_t *pp, int n)
{
    c->animus.vires     = par_da_int(pp, n, "vires", 0);
    c->animus.vitalitas = par_da_int(pp, n, "vitalitas", 0);
    const char *m = par_da_chordam(pp, n, "mens", "");
    if (m[0])
        snprintf(c->animus.mens, MENS_MAX, "%s", m);
}
