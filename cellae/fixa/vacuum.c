/*
 * vacuum.c — cella vacua
 */

#include "cella.h"

static actio_t vacuum_cogito(const struct tabula *tab, int x, int y)
{
    (void)tab;
    (void)x;
    (void)y;
    return ACTIO_NIHIL;
}

void vacuum_initia(void)
{
    cella_initia_ops(VACUUM, "\xC2\xB7 ", '.');

    genera_ops[VACUUM].cogito = vacuum_cogito;
}
