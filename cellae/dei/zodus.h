/*
 * zodus.h — zodus, genus phylo DEI — lusor sagittis gubernat (PELLE solum)
 */

#ifndef ZODUS_H
#define ZODUS_H

typedef struct {
    int imperium_num;       /* numerus imperiorum datorum */
} zodus_t;

/* directio imperata a lusore (directio_t = int enum) */
extern int zodus_imperium;

struct tabula;
void zodus_initia(void);

#endif /* ZODUS_H */
