/*
 * ursus.h — ursus qui feles edit
 */

#ifndef URSUS_H
#define URSUS_H

typedef struct {
    int ferocitas;          /* ferocitatem ursi */
} ursus_t;

struct tabula;
void ursus_initia(void);
void ursus_praecogita(struct tabula *tab);

#endif /* URSUS_H */
