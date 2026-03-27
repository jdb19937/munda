/*
 * fixum.h — classis fundamentalis phylo FIXUM
 *
 * Cellulae fixae: vacuum, saxum, murus.
 */

#ifndef FIXUM_H
#define FIXUM_H

typedef struct {
    int placeholder;        /* stub — expandetur */
} fixum_t;

struct cella;
struct json_par;
void fixum_praepara(struct cella *c, const struct json_par *pp, int n);

#endif /* FIXUM_H */
