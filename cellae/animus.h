/*
 * animus.h — classis phylorum ANIMA et DEI
 *
 * Omnis cellula animata (feles, ursus, dalekus, zodus) haec
 * attributa communia habet.
 */

#ifndef ANIMUS_H
#define ANIMUS_H

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
    int satietas;
    int vires;              /* quantum ponderis pellere possit */
    int vitalitas;          /* quantum impetum sustinere possit */
    char audita[AUDITA_MAX];
    char mens[MENS_MAX];
} animus_t;

/*
 * animus_praepara — legit vires et vitalitas ex paribus JSON.
 * vocatur post cella_praepara.
 */
struct cella;
struct json_par;
void animus_praepara(struct cella *c, const struct json_par *pp, int n);

#endif /* ANIMUS_H */
