/*
 * vide.c — ludus interactivus cum fenestra SDL2
 *
 * Similis lude.c, sed graphice per SDL2 pingit.
 * Usus: ./vide [munda [tempus_ms [mag_cellulae]]]
 *   munda         — via ad directorium mundi (praefinitum: mundae/imperium)
 *   tempus_ms     — intervallum inter gradus in ms (praefinitum: 100)
 *   mag_cellulae  — punctula per cellulam (praefinitum: 16)
 */

#include "cella.h"
#include "tabula.h"
#include "fenestra.h"
#include "oraculum.h"
#include "delphi/provisor.h"
#include "utilia.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <SDL.h>

/* definitio globalis tabulae operationum */
genus_ops_t genera_ops[GENERA_NUMERUS];

#define TEMPUS_PRAEFINITUM   100   /* ms */
#define MAG_PRAEFINITA        32   /* punctula per cellulam */

int main(int argc, char **argv)
{
    const char *munda = "mundae/imperium";
    int tempus_ms     = TEMPUS_PRAEFINITUM;
    int mag_cellulae  = MAG_PRAEFINITA;
    int argi          = 1;

    if (argi < argc)
        munda        = argv[argi++];
    if (argi < argc)
        tempus_ms    = atoi(argv[argi++]);
    if (argi < argc)
        mag_cellulae = atoi(argv[argi++]);
    if (tempus_ms < 10)
        tempus_ms = 10;
    if (mag_cellulae < 4)
        mag_cellulae = 4;

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

    /* initia oraculum */
    if (oraculum_initia() < 0)
        MORIRE("oraculum initiari non potuit");

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

    /* initia fenestram SDL */
    if (fenestra_initia(mag_cellulae) < 0)
        MORIRE("fenestra SDL initiari non potuit");

    /* pictura prima */
    fenestra_pinge(tab);

    /* ansa principalis */
    Uint32 tempus_praeteritum = SDL_GetTicks();

    while (!fenestra_clausa()) {

        /* lege eventus */
        int ch = fenestra_lege();
        if (ch == 'q')
            break;
        if (ch == 't')
            zodus_teleporta = 1;
        if (ch == 'A')
            zodus_imperium = SEPTENTRIO;
        if (ch == 'B')
            zodus_imperium = MERIDIES;
        if (ch == 'C')
            zodus_imperium = ORIENS;
        if (ch == 'D')
            zodus_imperium = OCCIDENS;

        /* gradus simulationis cum tempore fixo */
        Uint32 nunc = SDL_GetTicks();
        if ((int)(nunc - tempus_praeteritum) >= tempus_ms) {
            tempus_praeteritum = nunc;

            dalekus_praecogita(tab);
            ursus_praecogita(tab);
            corvus_praecogita(tab);

            tabula_gradus(tab);
            fenestra_pinge(tab);
        }

        /* ne CPU consumatur */
        SDL_Delay(1);
    }

    /* mundatio */
    fenestra_fini();
    oraculum_fini();
    tabula_libera(tab);

    return 0;
}
