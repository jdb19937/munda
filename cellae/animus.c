/*
 * animus.c — functiones communes phylo ANIMA
 */

#include "cella.h"

void animus_praepara(cella_t *c)
{
    c->animus.vires     = 0;
    c->animus.vitalitas = 0;
    c->animus.satietas  = 0;
}
