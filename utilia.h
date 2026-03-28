/*
 * utilia.h — functiones utiles communes
 */

#ifndef UTILIA_H
#define UTILIA_H

#include <stddef.h>
#include <stdnoreturn.h>
#include "json.h"

/* --- ANSI colores et stili --- */

#define ANSI_RST  "\033[0m"
#define ANSI_BLD  "\033[1m"
#define ANSI_DIM  "\033[2m"
#define ANSI_RED  "\033[31m"
#define ANSI_GRN  "\033[32m"
#define ANSI_YEL  "\033[33m"
#define ANSI_MAG  "\033[35m"
#define ANSI_CYN  "\033[36m"

/*
 * quaere primam occurrentiam cuiuslibet chordae ex serie in textu.
 * reddit indicem chordae inventae, vel -1 si nulla inventa.
 */
int prima_occurrentia(const char *textus,
                      const char *const *chordae, int num);

/*
 * scribe nuntium erroris ad stderr et exi.
 * includit nomen fasciculi et numerum lineae.
 */
noreturn void morire(const char *fasciculus, int linea,
                     const char *fmt, ...);

/* macro: morire cum __FILE__ et __LINE__ automatice */
#define MORIRE(...) morire(__FILE__, __LINE__, __VA_ARGS__)

/*
 * lege chordam modelli: "openai/gpt-5.4+high"
 *   → provisor: "openai" (vel "" si absens)
 *   → nomen: "gpt-5.4"
 *   → conatus:  "high" (vel "" si absens)
 * omnia scribuntur in pulvinaria data. spec non mutatur.
 */
void lege_sapientum(const char *spec,
                    char *provisor, size_t pmag,
                    char *nomen, size_t nmag,
                    char *conatus,  size_t cmag);

/*
 * lege totum fasciculum in memoriam. vocans liberet per free().
 * reddit NULL si error.
 */
/* --- lexicon: dictionarium clavis(chorda) → valor(long) --- */

typedef struct {
    char clavis[64];
    long valor;
} dictum_t;

typedef struct {
    dictum_t *dicta;
    int num;
    int cap;
} lexicon_t;

lexicon_t *lexicon_crea(void);
void lexicon_libera(lexicon_t *lex);
long lexicon_da(const lexicon_t *lex, const char *clavis);
void lexicon_pone(lexicon_t *lex, const char *clavis, long valor);
void lexicon_adde(lexicon_t *lex, const char *clavis, long additum);
int lexicon_numerus(const lexicon_t *lex);
const dictum_t *lexicon_dictum(const lexicon_t *lex, int index);

/* lexicon cum clavibus compositis "prima:secunda" */
void lexicon_adde_compositam(lexicon_t *lex, const char *prima,
                              const char *secunda, long additum);
long lexicon_da_compositam(const lexicon_t *lex, const char *prima,
                            const char *secunda);

/* enumera genera unica (partes primas ante ':') in tabulam */
int lexicon_genera(const lexicon_t *lex, char genera[][64], int max_genera);

/* --- fasciculi --- */

char *lege_fasciculum(const char *via);

/*
 * lege_instructiones — legit fasciculum .md ex mundo.
 * construit "{munda}/{phylum}/{genus}.md", legit, monet si absens.
 */
char *lege_instructiones(const char *munda, const char *phylum,
                         const char *genus);

/*
 * lege fasciculum .json iuxta fasciculum .c fontem et extrahe parametra.
 * reddit numerum parium lectorum. pares scribitur in json_par_t tabulam.
 * e.g. si fasciculus_c = "cellae/animae/ursus.c", legit "cellae/animae/ursus.json"
 */
int lege_parametra(const char *fasciculus_c, json_par_t *pares, int max);

/* da valorem numericam ex paribus per clavem, vel praefinitum si non inventa */
int par_da_int(const json_par_t *pares, int num,
               const char *clavis, int praefinitum);

/* da valorem chordae ex paribus — reddit in pulvinar statum, vel praefinitum */
const char *par_da_chordam(const json_par_t *pares, int num,
                           const char *clavis, const char *praefinitum);

#endif /* UTILIA_H */
