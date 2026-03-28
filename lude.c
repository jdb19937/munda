/*
 * lude.c — ludus interactivus cum terminali VT100
 *
 * Usus: ./lude [munda [tempus_ms]]
 *   munda     — via ad directorium mundi (praefinitum: mundae/imperium)
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

#define TEMPUS_PRAEFINITUM   100   /* ms */

int main(int argc, char **argv)
{
    const char *munda = "mundae/imperium";
    int tempus_ms = TEMPUS_PRAEFINITUM;
    int argi      = 1;

    if (argi < argc) munda     = argv[argi++];
    if (argi < argc) tempus_ms = atoi(argv[argi++]);
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
    corvus_initia();

    /* initia oraculum (crispus) pro dalekis et ursis */
    if (oraculum_initia() < 0)
        MORIRE("oraculum initiari non potuit");

    /* registra provisorem fictum (requirit genera_ops) */
    oraculum_adde_provisorem(&provisor_fictus);

    /* semen fortunae */
    srand((unsigned)time(NULL));

    tabula_t *tab = tabula_crea(munda);
    if (!tab)
        MORIRE("memoria defecit");
    tabula_imple(tab);

    /* crea zodum lusori si nondum existit */
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
                        if (abs(dx) != r && abs(dy) != r) continue;
                        int x = (zx + dx + tab->latus) % tab->latus;
                        int y = (zy + dy + tab->latus) % tab->latus;
                        if (tabula_da_const(tab, x, y)->genus == VACUUM) {
                            tabula_pone(tab, x, y, ZODUS);
                            snprintf(tabula_da(tab, x, y)->deus.nomen,
                                     sizeof(tabula_da(tab, x, y)->deus.nomen),
                                     "Zodus");
                            habet_zodum = 1;
                        }
                    }
        }
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
            int finis = 0;
            int ch;
            while ((ch = terminalis_lege()) >= 0) {
                if (ch == 'q' || ch == 'Q') {
                    finis = 1;
                    break;
                }
                if (ch == 't' || ch == 'T')
                    zodus_teleporta = 1;
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
        corvus_praecogita(tab);

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
