/*
 * fenestra.h — stratum graphicum SDL2
 *
 * Fenestra SDL pro pictura tabulae cum coloribus.
 * Substituit terminalem pro lusore graphico.
 */

#ifndef FENESTRA_H
#define FENESTRA_H

struct tabula;

/* initia fenestram SDL — reddit 0 si successum, -1 si erratum */
int fenestra_initia(int latus_cellulae);

/* pinge tabulam in fenestram */
void fenestra_pinge(const struct tabula *tab);

/* lege eventus SDL — reddit characterem vel -1 si nihil */
int fenestra_lege(void);

/* claude fenestram et munda SDL */
void fenestra_fini(void);

/* an fenestra clausa sit */
int fenestra_clausa(void);

#endif /* FENESTRA_H */
