/*
 * cella_ops.c — functiones hierarchicae pro parametris cellae
 */

#include "cella.h"
#include <string.h>

/* pulvinar status pro pictura per genus */
static char picturae[GENERA_NUMERUS][64];

void cella_initia_ops(genus_t genus, const char *pictura, char signum)
{
    strncpy(picturae[genus], pictura, 63);
    genera_ops[genus].pictura = picturae[genus];
    genera_ops[genus].signum  = signum;
}
