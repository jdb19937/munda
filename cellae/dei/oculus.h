/*
 * oculus.h — oculus, deus omnividens qui per oraculum cogitat
 */

#ifndef OCULUS_H
#define OCULUS_H

typedef struct {
    int visus_radius;       /* quantum longe videat */
} oculus_t;

struct tabula;
void oculus_initia(void);
void oculus_praecogita(struct tabula *tab);

#endif /* OCULUS_H */
