/*
 * cella_ops.c — initia genera_ops ex GENERA[] constante
 */

#include "cella.h"
#include "genera.h"

void cella_initia_ops(genus_t genus, const char *pictura, char signum)
{
    (void)pictura;
    (void)signum;
    genera_ops[genus].pictura     = GENERA[genus].pictura;
    genera_ops[genus].signum      = GENERA[genus].signum;
    genera_ops[genus].phylum      = GENERA[genus].phylum;
    genera_ops[genus].capacitates = GENERA[genus].capacitates;
}
