/*
 * animus.c — functiones communes phylo ANIMA
 */

#include "cella.h"

void animus_praepara(cella_t *c)
{
    c->p.animus.vires     = 0;
    c->p.animus.vitalitas = 0;
    c->p.animus.satietas  = 0;
}
