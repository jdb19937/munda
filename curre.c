/*
 * curre.c — cursus simulationis sine terminali
 *
 * Usus: ./munda [--obsidium] [latus [gradus]]
 *   latus   — longitudo lateris tabulae (praefinitum: 10)
 *   gradus  — numerus graduum simulationis (praefinitum: 20)
 */

#include "cella.h"
#include "tabula.h"
#include "oraculum.h"
#include "oracula/provisor.h"
#include "utilia.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* definitio globalis tabulae operationum */
genus_ops_t genera_ops[GENERA_NUMERUS];

#define LATUS_PRAEFINITUM    10
#define GRADUS_PRAEFINITUM   20

/* rationes generum — millesimae (e 1000) */
typedef struct {
    genus_t genus;
    int millesimae;
    const char *praefixum;
} ratio_generis_t;

static const ratio_generis_t rationes[] = {
    { SAXUM,     100, NULL },
    { FELES,      30, "F"  },
    { DALEKUS,    10, "A"  },
    { URSUS,      10, "U"  },
    { RAPUM,      20, NULL },
    { FUNGUS,     10, NULL },
};
#define RATIONES_NUM (int)(sizeof(rationes) / sizeof(rationes[0]))

/* X daleki ursum circumdent */
static void tabula_imple_obsidium(tabula_t *tab)
{
    int latus = tab->latus;

    for (int x = 0; x < latus; x++) {
        tabula_pone(tab, x, 0, MURUS);
        tabula_pone(tab, x, latus - 1, MURUS);
    }
    for (int y = 1; y < latus - 1; y++) {
        tabula_pone(tab, 0, y, MURUS);
        tabula_pone(tab, latus - 1, y, MURUS);
    }

    int cx = latus / 2, cy = latus / 2;
    tabula_pone(tab, cx, cy, URSUS);
    cella_t *u = tabula_da(tab, cx, cy);
    snprintf(u->animus.nomen, sizeof(u->animus.nomen), "U001");

    static const int dx[] = {-3, -2, 0, 2, 3, 3, 2, 0, -2, -3};
    static const int dy[] = { 0, -2,-3,-2, 0, 2, 3, 3,  3,  2};
    for (int i = 0; i < 10; i++) {
        int ax = cx + dx[i], ay = cy + dy[i];
        if (ax < 1 || ax >= latus-1 || ay < 1 || ay >= latus-1)
            continue;
        tabula_pone(tab, ax, ay, DALEKUS);
        cella_t *a = tabula_da(tab, ax, ay);
        snprintf(a->animus.nomen, sizeof(a->animus.nomen),
                 "A%03d", i + 1);
    }

    for (int i = 0; i < 8; i++) {
        int sx = 1 + rand() % (latus - 2);
        int sy = 1 + rand() % (latus - 2);
        if (tabula_da(tab, sx, sy)->genus == VACUUM)
            tabula_pone(tab, sx, sy, SAXUM);
    }
}

static void tabula_imple(tabula_t *tab)
{
    int latus = tab->latus;
    int indices[GENERA_NUMERUS] = {0};

    for (int x = 0; x < latus; x++) {
        tabula_pone(tab, x, 0, MURUS);
        tabula_pone(tab, x, latus - 1, MURUS);
    }
    for (int y = 1; y < latus - 1; y++) {
        tabula_pone(tab, 0, y, MURUS);
        tabula_pone(tab, latus - 1, y, MURUS);
    }

    for (int y = 1; y < latus - 1; y++) {
        for (int x = 1; x < latus - 1; x++) {
            int sors = rand() % 1000;
            int summa = 0;
            for (int r = 0; r < RATIONES_NUM; r++) {
                summa += rationes[r].millesimae;
                if (sors < summa) {
                    tabula_pone(tab, x, y, rationes[r].genus);
                    if (rationes[r].praefixum) {
                        cella_t *c = tabula_da(tab, x, y);
                        snprintf(c->animus.nomen, sizeof(c->animus.nomen),
                                 "%s%03d", rationes[r].praefixum,
                                 ++indices[rationes[r].genus]);
                    }
                    break;
                }
            }
        }
    }
}

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

    printf("F:%d U:%d B:%d r:%d f:%d\n\n",
           census[FELES], census[URSUS], census[DALEKUS],
           census[RAPUM], census[FUNGUS]);
}

int main(int argc, char **argv)
{
    int latus   = LATUS_PRAEFINITUM;
    int gradus  = GRADUS_PRAEFINITUM;
    int obsidium = 0;
    int argi     = 1;

    if (argi < argc && strcmp(argv[argi], "--obsidium") == 0) {
        obsidium = 1;
        argi++;
    }
    if (argi < argc) latus  = atoi(argv[argi++]);
    if (argi < argc) gradus = atoi(argv[argi++]);

    if (latus < 4)   latus = 4;
    if (latus > 128) latus = 128;
    if (gradus < 1)  gradus = 1;

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

    if (oraculum_initia() < 0)
        MORIRE("oraculum initiari non potuit");
    oraculum_adde_provisorem(&provisor_fictus);

    srand((unsigned)time(NULL));

    tabula_t *tab = tabula_crea(latus);
    if (!tab)
        MORIRE("memoria defecit");
    if (obsidium)
        tabula_imple_obsidium(tab);
    else
        tabula_imple(tab);

    if (!obsidium) {
        int cx = latus / 2, cy = latus / 2;
        tabula_pone(tab, cx, cy, ZODUS);
        cella_t *d = tabula_da(tab, cx, cy);
        snprintf(d->deus.nomen, sizeof(d->deus.nomen), "Zodus");
    }

    pinge(tab);

    for (int g = 0; g < gradus; g++) {
        dalekus_praecogita(tab);
        ursus_praecogita(tab);
        tabula_gradus(tab);
        pinge(tab);
    }

    oraculum_fini();
    tabula_libera(tab);
    return 0;
}
