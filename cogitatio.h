/*
 * cogitatio.h — praecogitatio generica per oraculum (asynchrona)
 *
 * Instructiones (instructiones systematis) = communia + specifica calleris.
 * Rogatum (rogatum usoris) = solum objectum ISON structuratum.
 */

#ifndef COGITATIO_H
#define COGITATIO_H

#include "cella.h"

#define PRAECOGITATA_MAX 256
#define PLICA_MAX   256
#define VOLANTES_MAX      32

typedef struct {
    int x, y;
    actio_t actio;
} praecogitatum_t;

typedef struct {
    int fossa;
    int xx[PLICA_MAX];
    int yy[PLICA_MAX];
    char nomina[PLICA_MAX][8];
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
                          int plica_mag, int patientia,
                          int radius,
                          const char *sapientum,
                          const char *instructiones,
                          praecogitata_t *res);

actio_t cogitatio_quaere(praecogitata_t *res, int x, int y);

/*
 * cogitatio_praecogita_tabulam — mittit totam tabulam ut ISON array.
 * cellulae generis dati ostenduntur per nomen, ceterae per signum generis.
 * rogatum continet: "tabula" (array duplex), "nomina" (lista nominum),
 * et statum cuiusque cellulae (ultima_actio, satietas, audita, mens).
 * responsum idem format: {"nomen": "actio", "nomen.mens": "cogitationes"}
 */
void cogitatio_praecogita_tabulam(struct tabula *tab, genus_t genus,
                                   const char *sapientum,
                                   const char *instructiones,
                                   praecogitata_t *res);

#endif /* COGITATIO_H */
