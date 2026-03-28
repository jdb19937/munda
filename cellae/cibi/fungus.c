/*
 * fungus.c — fungus edibilis
 */

#include "cella.h"

static void fungus_praepara(cella_t *c)
{
    c->pondus = 1;
}

void fungus_initia(void)
{
    cella_initia_ops(FUNGUS, "\xF0\x9F\x8D\x84", 'f');

    genera_ops[FUNGUS].praepara = fungus_praepara;
    genera_ops[FUNGUS].cogito   = NULL;
}
