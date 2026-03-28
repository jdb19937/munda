/*
 * tabula.h — tabula toroidalis quadrata
 *
 * Tabula est torus: margines involvuntur.
 * Omnis cellula quattuor vicinos habet.
 */

#ifndef TABULA_H
#define TABULA_H

#include "cella.h"

typedef struct tabula {
    int latus;                  /* longitudo lateris */
    cella_t *cellulae;          /* matrix latus × latus */
    unsigned long gradus_num;   /* numerus graduum peractorum */
    const char *munda;          /* via ad directorium mundi, e.g. "mundae/imperium" */
    char sapientum[GENERA_NUMERUS][64]; /* sapientum oraculi per genus */
} tabula_t;

/* crea tabulam ex {munda}/tabula.json — legit latus, omnes cellulae VACUUM */
tabula_t *tabula_crea(const char *munda);

/* imple tabulam ex fasciculis JSONL in munda */
void tabula_imple(tabula_t *tab);

/* libera memoriam */
void tabula_libera(tabula_t *tab);

/* da cellulam ad (x, y) — torus involvit coordinatas */
cella_t *tabula_da(tabula_t *tab, int x, int y);
const cella_t *tabula_da_const(const tabula_t *tab, int x, int y);

/* computa coordinatas vicini in directione data */
void tabula_vicinum(const tabula_t *tab, int x, int y, directio_t dir,
                    int *vx, int *vy);

/* pone cellulam generis dati ad (x, y) */
void tabula_pone(tabula_t *tab, int x, int y, genus_t genus);

/* unus gradus simulationis */
void tabula_gradus(tabula_t *tab);

#endif /* TABULA_H */
