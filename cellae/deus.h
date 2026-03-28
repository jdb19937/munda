/*
 * deus.h — classis fundamentalis phylo DEI
 *
 * Omnis deus (zodus, oculus, ...) haec attributa communia habet.
 * Deus NON est animus — phyla separata sunt.
 */

#ifndef DEUS_H
#define DEUS_H

#ifndef AUDITA_MAX
#define AUDITA_MAX 256
#endif
#ifndef MENS_MAX
#define MENS_MAX 64
#endif

typedef struct {
    char nomen[8];
    int ultima_modus;
    int ultima_directio;
    int ultima_permissa;
    int potentia;           /* vis divina */
    char audita[AUDITA_MAX];
    char mens[MENS_MAX];
    unsigned long mens_gradus;  /* gradus quo mens novissime mutata est */
} deus_t;

struct cella;
void deus_praepara(struct cella *c);

#endif /* DEUS_H */
