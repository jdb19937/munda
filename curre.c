/*
 * curre.c — cursus simulationis sine terminali
 *
 * Usus: ./curre [munda [gradus]]
 *   munda     — via ad directorium mundi (praefinitum: mundae/imperium)
 *   gradus    — numerus graduum simulationis (praefinitum: 20)
 */

#include "cella.h"
#include "tabula.h"
#include "oraculum.h"
#include "delphi/provisor.h"
#include "utilia.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* definitio globalis tabulae operationum */
genus_ops_t genera_ops[GENERA_NUMERUS];

#define GRADUS_PRAEFINITUM   20

/* pinge tabulam simpliciter ad stdout */
static void pinge(const tabula_t *tab)
{
    int latus = tab->latus;
    printf("--- gradus %lu ---\n", tab->gradus_num);
    for (int y = 0; y < latus; y++) {
        for (int x = 0; x < latus; x++) {
            const cella_t *c = tabula_da_const(tab, x, y);
            char s = genera_ops[c->genus].signum;
            putchar(s ? s : '.');
        }
        putchar('\n');
    }

    /* numera animas */
    int census[GENERA_NUMERUS] = {0};
    for (int y = 0; y < latus; y++)
        for (int x = 0; x < latus; x++)
            census[tabula_da_const(tab, x, y)->genus]++;

    printf(
        "F:%d U:%d B:%d C:%d r:%d f:%d\n\n",
        census[FELES], census[URSUS], census[DALEKUS],
        census[CORVUS], census[RAPUM], census[FUNGUS]
    );
}

int main(int argc, char **argv)
{
    const char *munda = "mundae/imperium";
    int gradus = GRADUS_PRAEFINITUM;
    int argi   = 1;

    if (argi < argc)
        munda  = argv[argi++];
    if (argi < argc)
        gradus = atoi(argv[argi++]);
    if (gradus < 1)
        gradus = 1;

    /* initia genera */
    memset(genera_ops, 0, sizeof(genera_ops));
    vacuum_initia();
    saxum_initia();
    feles_initia();
    dalekus_initia();
    ursus_initia();
    murus_initia();
    rapum_initia();
    fungus_initia();
    zodus_initia();
    oculus_initia();
    corvus_initia();

    if (oraculum_initia() < 0)
        MORIRE("oraculum initiari non potuit");
    oraculum_adde_provisorem(&provisor_fictus);

    srand((unsigned)time(NULL));

    tabula_t *tab = tabula_crea(munda);
    if (!tab)
        MORIRE("memoria defecit");
    tabula_imple(tab);

    /* crea zodum si nondum existit */
    {
        int habet_zodum = 0;
        for (int y = 0; y < tab->latus && !habet_zodum; y++)
            for (int x = 0; x < tab->latus && !habet_zodum; x++)
                if (tabula_da_const(tab, x, y)->genus == ZODUS)
                    habet_zodum = 1;
        if (!habet_zodum) {
            int zx = tab->latus / 2, zy = tab->latus / 2;
            for (int r = 0; r < tab->latus && !habet_zodum; r++)
                for (int dy = -r; dy <= r && !habet_zodum; dy++)
                    for (int dx = -r; dx <= r && !habet_zodum; dx++) {
                if (abs(dx) != r && abs(dy) != r)
                    continue;
                int x = (zx + dx + tab->latus) % tab->latus;
                int y = (zy + dy + tab->latus) % tab->latus;
                if (tabula_da_const(tab, x, y)->genus == VACUUM) {
                    tabula_pone(tab, x, y, ZODUS);
                    snprintf(
                        tabula_da(tab, x, y)->p.deus.nomen,
                        sizeof(tabula_da(tab, x, y)->p.deus.nomen),
                        "Zodus"
                    );
                    habet_zodum = 1;
                }
            }
        }
    }

    pinge(tab);

    for (int g = 0; g < gradus; g++) {
        dalekus_praecogita(tab);
        ursus_praecogita(tab);
        corvus_praecogita(tab);
        tabula_gradus(tab);
        pinge(tab);
    }

    oraculum_fini();
    tabula_libera(tab);
    return 0;
}
