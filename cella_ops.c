/*
 * cella_ops.c — functiones hierarchicae pro parametris cellae
 */

#include "cella.h"
#include "utilia.h"
#include <string.h>

/* pulvinar status pro pictura per genus */
static char picturae[GENERA_NUMERUS][64];

void cella_initia_ops(genus_t genus, const json_par_t *pp, int n)
{
    const char *pic = par_da_chordam(pp, n, "pictura", "??");
    const char *sig = par_da_chordam(pp, n, "signum", "?");

    strncpy(picturae[genus], pic, 63);

    genera_ops[genus].pictura = picturae[genus];
    genera_ops[genus].signum  = sig[0];
}

void cella_praepara(cella_t *c, genus_t genus,
                    const json_par_t *pp, int n)
{
    (void)genus;
    c->pondus = par_da_int(pp, n, "pondus", 0);
}
