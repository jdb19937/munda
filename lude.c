/*
 * lude.c — ludus interactivus cum terminali VT100
 *
 * Usus: ./munda [latus [tempus_ms]]
 *   latus     — longitudo lateris tabulae (praefinitum: 20)
 *   tempus_ms — intervallum inter gradus in ms (praefinitum: 100)
 */

#include "cella.h"
#include "tabula.h"
#include "terminalis.h"
#include "oraculum.h"
#include "oracula/provisor.h"
#include "utilia.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/select.h>
#include <errno.h>

/* definitio globalis tabulae operationum */
genus_ops_t genera_ops[GENERA_NUMERUS];

/* valores praefiniti */
#define LATUS_PRAEFINITUM    20
#define TEMPUS_PRAEFINITUM   100   /* ms */

/* rationes generum — millesimae (e 1000) */
typedef struct {
    genus_t genus;
    int millesimae;
    const char *praefixum;  /* praefixum nominis, vel NULL */
} ratio_generis_t;

static const ratio_generis_t rationes[] = {
    { SAXUM,     100, NULL },   /* 10% */
    { FELES,      30, "F"  },   /*  3% */
    { DALEKUS,    10, "A"  },   /*  1% */
    { URSUS,      10, "U"  },   /*  1% */
    { RAPUM,      20, NULL },   /*  2% */
    { FUNGUS,     10, NULL },   /*  1% */
};
#define RATIONES_NUM (int)(sizeof(rationes) / sizeof(rationes[0]))

/* X daleki ursum circumdent */
static void tabula_imple_obsidium(tabula_t *tab)
{
    int latus = tab->latus;

    /* murus circa margines */
    for (int x = 0; x < latus; x++) {
        tabula_pone(tab, x, 0, MURUS);
        tabula_pone(tab, x, latus - 1, MURUS);
    }
    for (int y = 1; y < latus - 1; y++) {
        tabula_pone(tab, 0, y, MURUS);
        tabula_pone(tab, latus - 1, y, MURUS);
    }

    /* ursus in centro */
    int cx = latus / 2, cy = latus / 2;
    tabula_pone(tab, cx, cy, URSUS);
    cella_t *u = tabula_da(tab, cx, cy);
    snprintf(u->animus.nomen, sizeof(u->animus.nomen), "U001");

    /* X daleki in circulo circa ursum */
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

    /* pauca saxa ut obstent */
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

    /* termina: murus circa margines */
    for (int x = 0; x < latus; x++) {
        tabula_pone(tab, x, 0, MURUS);
        tabula_pone(tab, x, latus - 1, MURUS);
    }
    for (int y = 1; y < latus - 1; y++) {
        tabula_pone(tab, 0, y, MURUS);
        tabula_pone(tab, latus - 1, y, MURUS);
    }

    /* imple interiora */
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
            /* aliter VACUUM manet */
        }
    }
}

int main(int argc, char **argv)
{
    int latus     = LATUS_PRAEFINITUM;
    int tempus_ms = TEMPUS_PRAEFINITUM;
    int obsidium  = 0;
    int argi      = 1;

    if (argi < argc && strcmp(argv[argi], "--obsidium") == 0) {
        obsidium = 1;
        argi++;
    }
    if (argi < argc) latus = atoi(argv[argi++]);
    if (argi < argc) tempus_ms = atoi(argv[argi++]);

    if (latus < 4)   latus = 4;
    if (latus > 128)  latus = 128;
    if (tempus_ms < 10) tempus_ms = 10;

    /* initia genera cellularum */
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

    /* initia oraculum (libcurl) pro dalekis et ursis */
    if (oraculum_initia() < 0)
        MORIRE("oraculum initiari non potuit");

    /* registra provisorem fictum (requirit genera_ops) */
    oraculum_adde_provisorem(&provisor_fictus);

    /* semen fortunae */
    srand((unsigned)time(NULL));

    /* crea et imple tabulam */
    tabula_t *tab = tabula_crea(latus);
    if (!tab)
        MORIRE("memoria defecit");
    if (obsidium)
        tabula_imple_obsidium(tab);
    else
        tabula_imple(tab);

    /* colloca zod in centro tabulae (nisi obsidium) */
    if (!obsidium) {
        int cx = latus / 2, cy = latus / 2;
        tabula_pone(tab, cx, cy, ZODUS);
        cella_t *d = tabula_da(tab, cx, cy);
        snprintf(d->deus.nomen, sizeof(d->deus.nomen), "Zodus");
    }

    /* installa signa et initia terminalem */
    signa_installa();
    if (terminalis_initia() < 0)
        MORIRE("terminalis non est tty");

    /* pictura prima */
    terminalis_pinge(tab);

    /* ansa principalis */
    while (!finis_opus) {

        /* si continuatio post SIGTSTP */
        if (continuatio_opus) {
            continuatio_opus = 0;
            terminalis_initia();
            repinge_opus = 1;
        }

        /* si repictura necessaria (SIGWINCH vel ^L) */
        if (repinge_opus) {
            repinge_opus = 0;
            /* munda screenum ante repicturam */
            write(STDOUT_FILENO, "\033[2J", 4);
            terminalis_pinge(tab);
        }

        /* exspecta inputum vel tempus */
        fd_set lecta;
        struct timeval mora;

        FD_ZERO(&lecta);
        FD_SET(STDIN_FILENO, &lecta);
        mora.tv_sec  = tempus_ms / 1000;
        mora.tv_usec = (tempus_ms % 1000) * 1000L;

        int res = select(STDIN_FILENO + 1, &lecta, NULL, NULL, &mora);

        if (res > 0) {
            /* exhauri omnes characteres pendentes, bufferiza sagittas */
            int finis = 0;
            int ch;
            while ((ch = terminalis_lege()) >= 0) {
                if (ch == 'q' || ch == 'Q') {
                    finis = 1;
                    break;
                }
                if (ch == 12) {
                    write(STDOUT_FILENO, "\033[2J", 4);
                    terminalis_pinge(tab);
                }
                if (ch == '\033') {
                    int ch2 = terminalis_lege();
                    if (ch2 == '[') {
                        int ch3 = terminalis_lege();
                        switch (ch3) {
                        case 'A': zodus_imperium = SEPTENTRIO; break;
                        case 'B': zodus_imperium = MERIDIES;   break;
                        case 'C': zodus_imperium = ORIENS;     break;
                        case 'D': zodus_imperium = OCCIDENS;   break;
                        }
                    }
                }
            }
            if (finis) break;
        } else if (res < 0 && errno != EINTR) {
            break;
        }

        /* praecogita dalekos et ursos ante gradum */
        dalekus_praecogita(tab);
        ursus_praecogita(tab);

        /* gradus simulationis */
        tabula_gradus(tab);

        /* pinge */
        terminalis_pinge(tab);
    }

    /* mundatio */
    terminalis_fini();
    oraculum_fini();
    tabula_libera(tab);

    return 0;
}
