/*
 * cogitatio.h — praecogitatio generica per oraculum (asynchrona)
 *
 * Instructiones (instructiones systematis) = communia + specifica calleris.
 * Rogatum (rogatum usoris) = solum objectum JSON structuratum.
 */

#ifndef COGITATIO_H
#define COGITATIO_H

#include "cella.h"

#define PRAECOGITATA_MAX 256
#define FASCICULUS_MAX   256
#define VOLANTES_MAX      32

typedef struct {
    int x, y;
    actio_t actio;
} praecogitatum_t;

typedef struct {
    int fossa;
    int xx[FASCICULUS_MAX];
    int yy[FASCICULUS_MAX];
    char nomina[FASCICULUS_MAX][8];
    int num;
} cogitatio_volans_t;

typedef struct {
    int x, y;
    char nomen[8];
} cogitatio_pendens_t;

typedef struct {
    cogitatio_pendens_t pendentes[PRAECOGITATA_MAX];
    int pendentes_num;
    int pendentes_gradus;

    cogitatio_volans_t volantes[VOLANTES_MAX];
    int volantes_num;

    praecogitatum_t acta[PRAECOGITATA_MAX];
    int num;
} praecogitata_t;

/*
 *   instructiones — textus specificus (e.g. "pelle saxa ad orientem")
 *                   appenditur ad system prompt communem
 */
void cogitatio_praecogita(struct tabula *tab, genus_t genus,
                          int limen, int modulus,
                          int fasciculus_mag, int patientia,
                          int radius,
                          const char *sapientum,
                          const char *instructiones,
                          praecogitata_t *res);

actio_t cogitatio_quaere(praecogitata_t *res, int x, int y);

#endif /* COGITATIO_H */
