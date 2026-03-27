/*
 * dalekus.h — dalekus (bot) quod oraculum groupnar rogat
 */

#ifndef DALEKUS_H
#define DALEKUS_H

typedef struct {
    int energia;            /* energia daleki */
} dalekus_t;

struct tabula;
void dalekus_initia(void);
void dalekus_praecogita(struct tabula *tab);

#endif /* DALEKUS_H */
