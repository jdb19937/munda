/*
 * rapum.c — rapum edibile
 */

#include "cella.h"

static void rapum_praepara(cella_t *c)
{
    c->pondus = 1;
}

void rapum_initia(void)
{
    cella_initia_ops(RAPUM, "\xF0\x9F\xA5\x95", 'r');

    genera_ops[RAPUM].praepara = rapum_praepara;
    genera_ops[RAPUM].cogito   = NULL;
}
