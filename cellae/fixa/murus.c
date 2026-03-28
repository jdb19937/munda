/*
 * murus.c — murus immobilis
 */

#include "cella.h"

static void murus_praepara(cella_t *c)
{
    c->pondus = 100;
}

void murus_initia(void)
{
    cella_initia_ops(MURUS, "\xE2\xAC\x9B", 'W');

    genera_ops[MURUS].praepara = murus_praepara;
    genera_ops[MURUS].cogito   = NULL;
}
