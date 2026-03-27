/*
 * cibus.h — classis fundamentalis phylo CIBUS
 *
 * Cellulae edibiles: rapum, fungus.
 */

#ifndef CIBUS_H
#define CIBUS_H

typedef struct {
    int nutritio;           /* quantum satietatis dat */
} cibus_t;

struct cella;
struct json_par;
void cibus_praepara(struct cella *c, const struct json_par *pp, int n);

#endif /* CIBUS_H */
