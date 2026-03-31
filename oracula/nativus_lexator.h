/*
 * nativus_lexator.h — lexator BPE nativus
 *
 * Tokenizator Byte-Pair Encoding (BPE) pro exercitatione et inferentiis.
 * Vocabularium ex corpore exercetur; deinde servatur/legitur ex plica.
 */

#ifndef NATIVUS_LEXATOR_H
#define NATIVUS_LEXATOR_H

#define NM_LEX_SIGNUM_MAGICUM 0x4E4D4C31u  /* 'NML1' */
#define NM_LEX_VOCABULUM_MAX  64           /* longitudo maxima vocabuli (chorda) */
#define NM_LEX_VOCAB_PRAEFIN  1024         /* magnitudo vocabularii praefinita */

/* ================================================================
 * vocabulum_t — unum vocabulum in vocabulario
 * ================================================================ */

typedef struct {
    char  chorda[NM_LEX_VOCABULUM_MAX];
    int   longitudo;   /* longitudo chordae in octettis */
    float gradus;      /* gradus mergi (ex exercitatione) */
} nm_vocabulum_t;

/* ================================================================
 * nm_lexator_t — lexator BPE
 * ================================================================ */

typedef struct {
    nm_vocabulum_t *vocabula;   /* array vocabulorum */
    int             vocab_magnitudo;
    int             vocab_max;
    int             max_vocab_lon;  /* longitudo maxima vocabuli (octetti) */
    /* vocabularium ordinatum pro quaesitione binaria */
    int            *ordo;       /* indicia in vocabula[], ordinata per chorda */
} nm_lexator_t;

/* ================================================================
 * functiones
 * ================================================================ */

/*
 * nm_lexator_crea — allocat lexatorem novum vacuum.
 * vocab_max: magnitudo vocabularii (praefinitum: NM_LEX_VOCAB_PRAEFIN).
 */
nm_lexator_t *nm_lexator_crea(int vocab_max);

/*
 * nm_lexator_destrue — liberat lexatorem.
 */
void nm_lexator_destrue(nm_lexator_t *lex);

/*
 * nm_lexator_exercita — aedificat vocabularium BPE ex corpore.
 * Incipit cum 256 signis octetticis; fundit paria frequentissima
 * donec vocab_max attingatur.
 * Reddit 0 si successum, -1 si error.
 */
int nm_lexator_exercita(nm_lexator_t *lex, const char *corpus);

/*
 * nm_lexator_serva — servat lexatorem in plicam binariam.
 */
int nm_lexator_serva(const nm_lexator_t *lex, const char *via);

/*
 * nm_lexator_lege — legit lexatorem ex plica binaria.
 * Reddit 0 si successum, -1 si error.
 */
int nm_lexator_lege(nm_lexator_t *lex, const char *via);

/*
 * nm_lexator_disseca — encodat textum in sequentiam signorum.
 * signa: buffer pro signis (vocans praebet; magnitudo >= lon_textus + 2).
 * n_signa: *n_signa imponitur numero signorum.
 * bos/eos: 1 = adde signum initii/finis.
 */
void nm_lexator_disseca(const nm_lexator_t *lex, const char *textus,
                        int bos, int eos, int *signa, int *n_signa);

/*
 * nm_lexator_redde — decodat signum in chorda.
 * reddit ptr in internas structuras (non libera).
 */
const char *nm_lexator_redde(const nm_lexator_t *lex, int signum);

#endif /* NATIVUS_LEXATOR_H */
