/*
 * visus.c — imago tabulae pro clientibus
 *
 * Deserializatio ISON et pictura terminalis.
 * Sine dependentia a cella.h, tabula.h, oraculum.h.
 */

#include "visus.h"
#include "ison.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* --- ANSI --- */

#define ANSI_RST "\033[0m"
#define ANSI_BLD "\033[1m"
#define ANSI_DIM "\033[2m"
#define ANSI_CYN "\033[36m"

/* --- hex ↔ genus --- */

static genus_t hex_ad_genus(char c)
{
    if (c >= '0' && c <= '9') return (genus_t)(c - '0');
    if (c >= 'a' && c <= 'f') return (genus_t)(c - 'a' + 10);
    return VACUUM;
}

/* --- vita/mors --- */

void visus_initia(visus_t *v)
{
    memset(v, 0, sizeof(*v));
}

void visus_libera(visus_t *v)
{
    free(v->genera);
    v->genera = NULL;
    v->latus = 0;
}

/* --- deserializatio --- */

/* iteremus per ISON array objectorum */
static void lege_entia(visus_t *v, const char *entia_ison)
{
    v->entia_num = 0;
    const char *p = entia_ison;

    while (*p && *p != '[') p++;
    if (*p == '[') p++;

    while (*p && v->entia_num < VISUS_ENTIA_MAX) {
        while (*p && *p != '{') {
            if (*p == ']') return;
            p++;
        }
        if (*p != '{') break;

        /* inveni finem objecti */
        const char *initium = p;
        int profunditas = 0;
        int in_chorda = 0;
        const char *q = p;
        while (*q) {
            if (*q == '"' && (q == initium || *(q-1) != '\\'))
                in_chorda = !in_chorda;
            if (!in_chorda) {
                if (*q == '{') profunditas++;
                if (*q == '}') {
                    profunditas--;
                    if (profunditas == 0) { q++; break; }
                }
            }
            q++;
        }

        size_t ens_mag = (size_t)(q - initium);
        char *ens = malloc(ens_mag + 1);
        if (!ens) break;
        memcpy(ens, initium, ens_mag);
        ens[ens_mag] = '\0';

        visus_ens_t *ve = &v->entia[v->entia_num];
        memset(ve, 0, sizeof(*ve));

        ve->x     = (int)ison_da_numerum(ens, "x");
        ve->y     = (int)ison_da_numerum(ens, "y");
        ve->genus = (genus_t)ison_da_numerum(ens, "g");

        char *nomen = ison_da_chordam(ens, "n");
        if (nomen) {
            snprintf(ve->nomen, VISUS_NOMEN_MAX, "%s", nomen);
            free(nomen);
        }

        phylum_t ph = GENERA[ve->genus].phylum;
        if (ph == ANIMA) {
            ve->satietas  = (int)ison_da_numerum(ens, "sa");
            ve->vires     = (int)ison_da_numerum(ens, "vi");
            ve->vitalitas = (int)ison_da_numerum(ens, "vt");
        } else if (ph == DEI) {
            ve->potentia  = (int)ison_da_numerum(ens, "po");
        }

        ve->ultima_modus    = (int)ison_da_numerum(ens, "um");
        ve->ultima_directio = (int)ison_da_numerum(ens, "ud");
        ve->ultima_permissa = (int)ison_da_numerum(ens, "up");

        char *audita = ison_da_chordam(ens, "au");
        if (audita) {
            snprintf(ve->audita, VISUS_AUDITA_MAX, "%s", audita);
            free(audita);
        }
        char *mens = ison_da_chordam(ens, "me");
        if (mens) {
            snprintf(ve->mens, VISUS_MENS_MAX, "%s", mens);
            free(mens);
        }

        v->entia_num++;
        free(ens);
        p = q;
    }
}

int visus_ex_ison(visus_t *v, const char *ison, size_t mag)
{
    (void)mag;

    int latus = (int)ison_da_numerum(ison, "latus");
    if (latus <= 0 || latus > 256) return -1;

    /* realloca si latus mutat */
    if (v->latus != latus) {
        free(v->genera);
        v->latus = latus;
        v->genera = calloc((size_t)(latus * latus), sizeof(genus_t));
        if (!v->genera) return -1;
    }

    v->gradus = (unsigned long)ison_da_numerum(ison, "gradus");

    /* genera chorda */
    char *genera_str = ison_da_chordam(ison, "genera");
    if (!genera_str) return -1;

    int cellulae_num = latus * latus;
    if ((int)strlen(genera_str) != cellulae_num) {
        free(genera_str);
        return -1;
    }

    for (int i = 0; i < cellulae_num; i++)
        v->genera[i] = hex_ad_genus(genera_str[i]);
    free(genera_str);

    /* entia */
    char *entia_ison = ison_da_crudum(ison, "entia");
    if (entia_ison) {
        lege_entia(v, entia_ison);
        free(entia_ison);
    } else {
        v->entia_num = 0;
    }

    return 0;
}

/* --- pictor terminalis --- */

void visus_pinge(const visus_t *v)
{
    int latus = v->latus;
    if (latus <= 0 || !v->genera) return;

    size_t buf_mag = 4096 + (size_t)(latus * latus) * 16
                     + (size_t)v->entia_num * 256;
    char *buf = malloc(buf_mag);
    if (!buf) return;
    char *p = buf;

    /* cursor domum */
    p += sprintf(p, "\033[H");

    /* margo superior */
    p += sprintf(p, "\u250C");
    for (int x = 0; x < latus; x++)
        p += sprintf(p, "\u2500\u2500");
    p += sprintf(p, "\u2510\n");

    /* cellulae */
    for (int y = 0; y < latus; y++) {
        p += sprintf(p, "\u2502");
        for (int x = 0; x < latus; x++) {
            genus_t g = v->genera[y * latus + x];
            const char *pic = GENERA[g].pictura;
            p += sprintf(p, "%s", pic ? pic : "  ");
        }
        p += sprintf(p, "\u2502\n");
    }

    /* margo inferior */
    p += sprintf(p, "\u2514");
    for (int x = 0; x < latus; x++)
        p += sprintf(p, "\u2500\u2500");
    p += sprintf(p, "\u2518\n");

    /* linea status */
    p += sprintf(p, " gradus: %lu   latus: %d\033[K\n\033[K\n",
                 v->gradus, latus);

    /* mentes entium */
    p += sprintf(p, ANSI_BLD "── mentes ──" ANSI_RST "\033[K\n");

    /* ordina entia per nomen (insertio simplex in copia) */
    int *ordo = malloc((size_t)v->entia_num * sizeof(int));
    if (ordo) {
        for (int i = 0; i < v->entia_num; i++) ordo[i] = i;
        for (int i = 1; i < v->entia_num; i++) {
            int tmp = ordo[i];
            int j = i - 1;
            while (j >= 0 && strcmp(v->entia[ordo[j]].nomen,
                                    v->entia[tmp].nomen) > 0) {
                ordo[j + 1] = ordo[j];
                j--;
            }
            ordo[j + 1] = tmp;
        }

        static const char *dir_signa[] = {"\xC2\xB7",
            "\xE2\x86\x91", "\xE2\x86\x93",
            "\xE2\x86\x90", "\xE2\x86\x92"};

        for (int i = 0; i < v->entia_num; i++) {
            const visus_ens_t *e = &v->entia[ordo[i]];
            const char *pic = GENERA[e->genus].pictura;
            if (!pic) pic = "??";
            int d = e->ultima_directio;
            const char *ds = (d >= 0 && d <= 4) ? dir_signa[d] : "?";
            const char *ps = e->ultima_permissa ? "" : "\xE2\x9C\x97";

            if (e->mens[0]) {
                p += sprintf(p, " %s " ANSI_CYN "%-4s" ANSI_RST
                             " " ANSI_DIM "{%s%s} — %s" ANSI_RST "\033[K\n",
                             pic, e->nomen, ds, ps, e->mens);
            } else {
                p += sprintf(p, " %s " ANSI_CYN "%-4s" ANSI_RST
                             " " ANSI_DIM "{%s%s} —" ANSI_RST "\033[K\n",
                             pic, e->nomen, ds, ps);
            }
        }
        free(ordo);
    }

    /* purga */
    for (int i = 0; i < 4; i++)
        p += sprintf(p, "\033[K\n");

    write(STDOUT_FILENO, buf, (size_t)(p - buf));
    free(buf);
}

/* --- pictor simplex (ASCII) --- */

void visus_pinge_simplex(const visus_t *v)
{
    int latus = v->latus;
    if (latus <= 0 || !v->genera) return;

    printf("\033[H\033[2J");
    printf("--- gradus %lu ---\n", v->gradus);
    for (int y = 0; y < latus; y++) {
        for (int x = 0; x < latus; x++) {
            genus_t g = v->genera[y * latus + x];
            char s = GENERA[g].signum;
            putchar(s ? s : '.');
        }
        putchar('\n');
    }
    fflush(stdout);
}
