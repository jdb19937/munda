/*
 * nativus_lexator.c — lexator BPE nativus
 *
 * Byte-Pair Encoding: initiat ex 256 signis octetticis,
 * fundit paria frequentissima donec vocab_max attingatur.
 */

#include "nativus_lexator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ================================================================
 * auxiliaria interna
 * ================================================================ */

/*
 * quaere_vocabulum — quaerit chorda in vocabulario ordinato.
 * reddit indicem in vocabula[] vel -1.
 */
static int quaere_vocabulum(const nm_lexator_t *lex, const char *chorda)
{
    int lo = 0, hi = lex->vocab_magnitudo - 1;
    while (lo <= hi) {
        int med = (lo + hi) / 2;
        int cmp = strcmp(lex->vocabula[lex->ordo[med]].chorda, chorda);
        if (cmp == 0)
            return lex->ordo[med];
        if (cmp < 0)
            lo = med + 1;
        else
            hi = med - 1;
    }
    return -1;
}

/*
 * compara_vocabula — comparator pro qsort (per chorda).
 */
static nm_lexator_t *g_lex_ordinatio; /* globale auxiliare pro qsort */

static int compara_vocabula(const void *a, const void *b)
{
    int ia = *(const int *)a, ib = *(const int *)b;
    return strcmp(
        g_lex_ordinatio->vocabula[ia].chorda,
        g_lex_ordinatio->vocabula[ib].chorda
    );
}

/*
 * aedifica_ordinem — (re)aedificat ordo[] post mutationes vocabula[].
 */
static void aedifica_ordinem(nm_lexator_t *lex)
{
    for (int i = 0; i < lex->vocab_magnitudo; i++)
        lex->ordo[i] = i;
    g_lex_ordinatio = lex;
    qsort(lex->ordo, lex->vocab_magnitudo, sizeof(int), compara_vocabula);
}

/* ================================================================
 * crea / destrue
 * ================================================================ */

nm_lexator_t *nm_lexator_crea(int vocab_max)
{
    nm_lexator_t *lex = calloc(1, sizeof(*lex));
    if (!lex)
        return NULL;
    lex->vocab_max = vocab_max > 0 ? vocab_max : NM_LEX_VOCAB_PRAEFIN;
    lex->vocabula  = calloc(lex->vocab_max, sizeof(nm_vocabulum_t));
    lex->ordo      = calloc(lex->vocab_max, sizeof(int));
    if (!lex->vocabula || !lex->ordo) {
        free(lex->vocabula);
        free(lex->ordo);
        free(lex);
        return NULL;
    }
    lex->vocab_magnitudo = 0;
    return lex;
}

void nm_lexator_destrue(nm_lexator_t *lex)
{
    if (!lex)
        return;
    free(lex->vocabula);
    free(lex->ordo);
    free(lex);
}

/* ================================================================
 * exercitatio BPE
 * ================================================================ */

int nm_lexator_exercita(nm_lexator_t *lex, const char *corpus)
{
    if (!lex || !corpus)
        return -1;

    /* --- initia: 256 signa pro singulis octetis --- */
    for (int i = 0; i < 256; i++) {
        lex->vocabula[i].chorda[0] = (char)i;
        lex->vocabula[i].chorda[1] = '\0';
        lex->vocabula[i].longitudo = 1;
        lex->vocabula[i].gradus    = 0.0f;
    }
    lex->vocab_magnitudo = 256;
    aedifica_ordinem(lex);

    /* --- corpus in signa initialia disseca --- */
    int n      = (int)strlen(corpus);
    int *signa = malloc((size_t)n * sizeof(int));
    if (!signa)
        return -1;
    for (int i = 0; i < n; i++)
        signa[i] = (unsigned char)corpus[i];

    /* buffer pro contis (par = duo signa contigua) */
    /* maximus parium: n - 1; adhibemus tabulam frequentiarum 2D: */
    /* quoniam vocab_max <= 512, tabula VM*VM = 262144 ints = 1MB — acceptabile */
    int VM    = lex->vocab_max;
    int *freq = calloc((size_t)VM * VM, sizeof(int));
    if (!freq) {
        free(signa);
        return -1;
    }

    /* --- ansa fusionis BPE --- */
    while (lex->vocab_magnitudo < lex->vocab_max) {
        /* pone omnes frequentias in 0 */
        memset(freq, 0, (size_t)VM * VM * sizeof(int));

        /* numeratio parium */
        for (int i = 0; i < n - 1; i++) {
            if (signa[i] < 0 || signa[i+1] < 0)
                continue;
            freq[(size_t)signa[i] * VM + signa[i+1]]++;
        }

        /* inveni par frequentissimum */
        int best_a = -1, best_b = -1, best_freq = 0;
        for (int a = 0; a < lex->vocab_magnitudo; a++)
            for (int b = 0; b < lex->vocab_magnitudo; b++) {
            int f = freq[(size_t)a * VM + b];
            if (f > best_freq) {
                best_freq = f;
                best_a    = a;
                best_b    = b;
            }
        }

        if (best_freq <= 1)
            break; /* nihil fundit */

        /* crea vocabulum fusionem */
        int nou = lex->vocab_magnitudo;
        int la  = lex->vocabula[best_a].longitudo;
        int lb  = lex->vocabula[best_b].longitudo;
        if (la + lb >= NM_LEX_VOCABULUM_MAX)
            break;
        memcpy(
            lex->vocabula[nou].chorda,
            lex->vocabula[best_a].chorda, la
        );
        memcpy(
            lex->vocabula[nou].chorda + la,
            lex->vocabula[best_b].chorda, lb + 1
        );
        lex->vocabula[nou].longitudo = la + lb;
        lex->vocabula[nou].gradus    = (float)best_freq;
        lex->vocab_magnitudo++;

        /* actualiza max_vocab_lon */
        if (la + lb > lex->max_vocab_lon)
            lex->max_vocab_lon = la + lb;

        /* applica fusionem in signa */
        int j = 0;
        for (int i = 0; i < n; ) {
            if (i < n - 1 && signa[i] == best_a && signa[i+1] == best_b) {
                signa[j++] = nou;
                i += 2;
            } else {
                signa[j++] = signa[i++];
            }
        }
        n = j;

        /* (re)aedifica ordinem post additamentum */
        aedifica_ordinem(lex);
    }

    free(freq);
    free(signa);
    return 0;
}

/* ================================================================
 * serva / lege
 * ================================================================ */

int nm_lexator_serva(const nm_lexator_t *lex, const char *via)
{
    FILE *f = fopen(via, "wb");
    if (!f)
        return -1;

    unsigned int magicus = NM_LEX_SIGNUM_MAGICUM;
    fwrite(&magicus,              sizeof(unsigned int), 1, f);
    fwrite(&lex->vocab_magnitudo, sizeof(int),          1, f);
    fwrite(&lex->max_vocab_lon,   sizeof(int),          1, f);

    for (int i = 0; i < lex->vocab_magnitudo; i++) {
        fwrite(&lex->vocabula[i].gradus,    sizeof(float), 1, f);
        fwrite(&lex->vocabula[i].longitudo, sizeof(int),   1, f);
        fwrite(
            lex->vocabula[i].chorda,     1,
            (size_t)lex->vocabula[i].longitudo + 1, f
        );
    }

    fclose(f);
    return 0;
}

int nm_lexator_lege(nm_lexator_t *lex, const char *via)
{
    FILE *f = fopen(via, "rb");
    if (!f)
        return -1;

    unsigned int magicus;
    if (
        fread(&magicus, sizeof(unsigned int), 1, f) != 1 ||
        magicus != NM_LEX_SIGNUM_MAGICUM
    ) {
        fclose(f);
        return -1;
    }

    int vm, mvl;
    if (
        fread(&vm,  sizeof(int), 1, f) != 1 ||
        fread(&mvl, sizeof(int), 1, f) != 1 ||
        vm > lex->vocab_max
    ) {
        fclose(f);
        return -1;
    }
    lex->vocab_magnitudo = vm;
    lex->max_vocab_lon   = mvl;

    for (int i = 0; i < vm; i++) {
        float gradus;
        int lon;
        if (
            fread(&gradus, sizeof(float), 1, f) != 1 ||
            fread(&lon,    sizeof(int),   1, f) != 1 ||
            lon <= 0 || lon >= NM_LEX_VOCABULUM_MAX
        ) {
            fclose(f);
            return -1;
        }
        lex->vocabula[i].gradus    = gradus;
        lex->vocabula[i].longitudo = lon;
        if (fread(lex->vocabula[i].chorda, 1, (size_t)lon + 1, f) != (size_t)lon + 1) {
            fclose(f);
            return -1;
        }
    }

    fclose(f);
    aedifica_ordinem(lex);
    return 0;
}

/* ================================================================
 * disseca (encode)
 * ================================================================ */

void nm_lexator_disseca(
    const nm_lexator_t *lex, const char *textus,
    int bos, int eos, int *signa, int *n_signa
) {
    int n = 0;

    if (bos)
        signa[n++] = 1; /* signum initii (conventio: index 1) */

    /* initia: signa pro singulis octetis */
    for (const unsigned char *p = (const unsigned char *)textus; *p; p++)
        signa[n++] = (int)*p;

    /* BPE: funde paria dum possibile */
    char par[NM_LEX_VOCABULUM_MAX * 2];
    int mutatum = 1;
    while (mutatum) {
        mutatum = 0;
        float optimum_gradus = -1e10f;
        int   optimum_idx    = -1;
        int   optimum_id     = -1;

        for (int i = 0; i < n - 1; i++) {
            /* construc chorda paris */
            int la = lex->vocabula[signa[i]].longitudo;
            int lb = lex->vocabula[signa[i+1]].longitudo;
            if (la + lb >= NM_LEX_VOCABULUM_MAX)
                continue;
            memcpy(par,      lex->vocabula[signa[i]].chorda,   la);
            memcpy(par + la, lex->vocabula[signa[i+1]].chorda, lb + 1);

            int id = quaere_vocabulum(lex, par);
            if (id >= 0 && lex->vocabula[id].gradus > optimum_gradus) {
                optimum_gradus = lex->vocabula[id].gradus;
                optimum_idx    = i;
                optimum_id     = id;
            }
        }

        if (optimum_idx < 0)
            break;

        /* applica fusionem */
        signa[optimum_idx] = optimum_id;
        for (int i = optimum_idx + 1; i < n - 1; i++)
            signa[i] = signa[i + 1];
        n--;
        mutatum = 1;
    }

    if (eos)
        signa[n++] = 2; /* signum finis (conventio: index 2) */
    *n_signa = n;
}

/* ================================================================
 * redde (decode)
 * ================================================================ */

const char *nm_lexator_redde(const nm_lexator_t *lex, int signum)
{
    if (signum < 0 || signum >= lex->vocab_magnitudo)
        return "?";
    return lex->vocabula[signum].chorda;
}
