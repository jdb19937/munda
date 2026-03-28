/*
 * corvus.h — corvus, avis callida et rapax
 */

#ifndef CORVUS_H
#define CORVUS_H

typedef struct {
    int audacia;            /* audacia corvi — quanto audacior, tanto minus fugit */
} corvus_t;

struct tabula;
void corvus_initia(void);
void corvus_praecogita(struct tabula *tab);

#endif /* CORVUS_H */
