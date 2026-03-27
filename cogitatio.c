/*
 * cogitatio.c — praecogitatio generica per oraculum (asynchrona)
 */

#include "cogitatio.h"
#include "tabula.h"
#include "oraculum.h"
#include "json.h"
#include "utilia.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

/* --- instructiones communes --- */

static const char *INSTRUCTIONES_COMMUNES =
    "Ludus tabulae es. Regis cellulas in tabula.\n\n"
    "Legenda: .=vacuum #=saxum W=murus F=feles B=dalekus U=ursus "
    "r=rapum f=fungus Z=zodus O=oculus @=tu\n\n"
    "VICINITAS ET DIRECTIONES:\n"
    "Campo \"vicinitas\" est JSON array duplex: vicinitas[series][columna].\n"
    "  series 0 = septentrio (sursum), ultima series = meridies (deorsum)\n"
    "  columna 0 = occidens (sinistrorsum), ultima columna = oriens (dextrorsum)\n"
    "  @ in centro es tu.\n"
    "Exemplum (radius 2, tu in [2][2]):\n"
    "  [[\".\",\".\",\".\",\".\",\".\"],\n"
    "   [\".\",\"#\",\".\",\"F\",\".\"],\n"
    "   [\".\",\".\",\"@\",\".\",\".\"],\n"
    "   [\".\",\".\",\".\",\".\",\"r\"],\n"
    "   [\".\",\".\",\"U\",\".\",\".\"]]\n"
    "In hoc exemplo: F est ad septentrio-oriens, "
    "U est ad meridies, r est ad meridies-oriens.\n"
    "DIRECTIO actionum = septentrio, meridies, oriens, occidens "
    "(solum quattuor cardinales, non diagonales).\n\n"
    "Rogatum est objectum JSON. Cuiusque clavis est nomen cellulae tuae. "
    "Valor est objectum cum:\n"
    "- \"vicinitas\": array duplex ut supra\n"
    "- \"ultima_actio\": ultima actio tentata et exitus, si habetur\n"
    "- \"satietas\": saturitas ex cibis editis\n\n"
    "Modi actionum:\n"
    "- move DIRECTIO: move ad vacuum tantum\n"
    "- pelle DIRECTIO: pelle obiectum ante te in ea directione\n"
    "- cape DIRECTIO: cape (ede) cibum, move in locum eius\n"
    "- trahe DIRECTIO: move ad vacuum, trahe obiectum a tergo tuo\n"
    "- oppugna DIRECTIO: neca animum vicinum (vires > vitalitas requirit)\n"
    "- loquere DIRECTIO verba: dic verba ad vicinum in ea directione "
    "(non moveris)\n"
    "- clama verba: clama ad omnes in vicinitate 3x3 "
    "(non moveris, directio non requiritur)\n"
    "- quiesce: nihil agere\n\n"
    "Si campo \"audita\" habes, continet verba quae alii tibi dixerunt.\n"
    "Campo \"mens\" continet cogitationes tuas priores.\n\n"
    "Responde objecto JSON: {\"nomen\": \"actio\", \"nomen.mens\": \"cogitationes\"}\n"
    "Clavis .mens est voluntaria. Si data, mentem tuam novam continet.\n"
    "Exempla: {\"U001\": \"oppugna septentrio\", \"U001.mens\": \"felem video\"}, "
    "{\"A001\": \"move oriens\"}, "
    "{\"A002\": \"loquere septentrio salve amice\", "
    "\"A002.mens\": \"amicum saluto\"}, "
    "{\"U003\": \"clama periculum adest!\"}\n\n";

/* signum characteris pro genere — ex genera_ops */
static char genus_signum(genus_t g)
{
    if (g >= 0 && g < GENERA_NUMERUS && genera_ops[g].signum)
        return genera_ops[g].signum;
    return '?';
}

/* aedifica vicinitatem ut JSON array duplex: [[".","."],["@","#"]] */
static void aedifica_vicinitatem(const tabula_t *tab, int cx, int cy,
                                 int radius, char *buf, size_t mag)
{
    char *p = buf;
    char *finis = buf + mag - 1;

    *p++ = '[';
    for (int dy = -radius; dy <= radius; dy++) {
        if (p >= finis) break;
        *p++ = '[';
        for (int dx = -radius; dx <= radius; dx++) {
            if (p + 4 >= finis) break;
            *p++ = '"';
            const cella_t *c = tabula_da_const(tab, cx + dx, cy + dy);
            *p++ = (dx == 0 && dy == 0) ? '@' : genus_signum(c->genus);
            *p++ = '"';
            if (dx < radius)
                *p++ = ',';
        }
        *p++ = ']';
        if (dy < radius)
            *p++ = ',';
    }
    *p++ = ']';
    *p = '\0';
}

/* directiones communes — adhibitae a parse_actionem */
static const char *const dir_nomina[] = {
    "septentrio", "meridies", "occidens", "oriens"
};
static const int dir_lon[] = {
    10, 8, 8, 6
};
static const directio_t dir_val[] = {
    SEPTENTRIO, MERIDIES, OCCIDENS, ORIENS
};
#define DIR_NUM 4

/* extrahe sermonem post positam in textu originali */
static void extrahe_sermonem(const char *val, size_t pos, char *sermo)
{
    const char *p = val + pos;
    while (*p == ' ') p++;
    snprintf(sermo, SERMO_MAX, "%s", p);
    /* recide spatia ad finem */
    size_t l = strlen(sermo);
    while (l > 0 && sermo[l - 1] == ' ') sermo[--l] = '\0';
}

/* extrahe actionem (modus + directio + sermo) ex valore responsi */
static actio_t parse_actionem(const char *val)
{
    char minus[256];
    size_t i;
    for (i = 0; val[i] && i < sizeof(minus) - 1; i++)
        minus[i] = (char)tolower((unsigned char)val[i]);
    minus[i] = '\0';

    /* loquere */
    {
        char *p = strstr(minus, "loquere");
        if (p) {
            size_t post = (size_t)(p - minus) + 7;
            actio_t res = { LOQUERE, DIR_NIHIL, {0}, {0} };
            int di = prima_occurrentia(minus + post, dir_nomina, DIR_NUM);
            if (di >= 0) {
                res.directio = dir_val[di];
                char *dp = strstr(minus + post, dir_nomina[di]);
                if (dp) {
                    size_t spos = (size_t)(dp - minus) + dir_lon[di];
                    extrahe_sermonem(val, spos, res.sermo);
                }
            }
            return res;
        }
    }

    /* clama */
    {
        char *p = strstr(minus, "clama");
        if (p) {
            size_t post = (size_t)(p - minus) + 5;
            actio_t res = { CLAMA, DIR_NIHIL, {0}, {0} };
            extrahe_sermonem(val, post, res.sermo);
            return res;
        }
    }

    /* lege modum */
    static const char *const modi_nomina[] = {
        "pelle", "cape", "trahe", "oppugna"
    };
    static const modus_t modi_val[] = {
        PELLE, CAPE, TRAHE, OPPUGNA
    };

    modus_t modus = MOVE;  /* praefinitum */
    int mi = prima_occurrentia(minus, modi_nomina, 4);
    if (mi >= 0)
        modus = modi_val[mi];

    if (strstr(minus, "quiesce"))
        return ACTIO_NIHIL;

    /* lege directionem */
    int di = prima_occurrentia(minus, dir_nomina, DIR_NUM);
    if (di < 0)
        return ACTIO_NIHIL;

    return (actio_t){ modus, dir_val[di], {0}, {0} };
}

/* an cellula (x,y) iam in processu sit */
static int iam_in_processu(const praecogitata_t *res, int x, int y)
{
    for (int i = 0; i < res->pendentes_num; i++)
        if (res->pendentes[i].x == x && res->pendentes[i].y == y)
            return 1;
    for (int v = 0; v < res->volantes_num; v++)
        for (int j = 0; j < res->volantes[v].num; j++)
            if (res->volantes[v].xx[j] == x && res->volantes[v].yy[j] == y)
                return 1;
    for (int i = 0; i < res->num; i++)
        if (res->acta[i].x == x && res->acta[i].y == y)
            return 1;
    return 0;
}

/* aedifica instructiones: communia + specifica */
static char *aedifica_instructiones(const char *specifica)
{
    size_t mag = strlen(INSTRUCTIONES_COMMUNES) + strlen(specifica) + 1;
    char *res = malloc(mag);
    if (!res) return NULL;
    snprintf(res, mag, "%s%s", INSTRUCTIONES_COMMUNES, specifica);
    return res;
}

/* aedifica valorem structuratum JSON pro una cellula:
 * {"vicinitas": [[...]], "ultimus_motus": "..., ..."} */
static void aedifica_valorem(const tabula_t *tab,
                             int x, int y, int radius,
                             char *buf, size_t mag)
{
    static const char *dir_nomina[] = {
        "nihil", "septentrio", "meridies", "occidens", "oriens"
    };
    static const char *modus_nomina[] = {
        "quiesce", "move", "pelle", "cape", "trahe", "loquere", "clama"
    };

    char vic[2048];
    aedifica_vicinitatem(tab, x, y, radius, vic, sizeof(vic));

    const cella_t *c = tabula_da_const(tab, x, y);
    int est_deus = (genera_ops[c->genus].phylum == DEI);
    int um = est_deus ? c->deus.ultima_modus    : c->animus.ultima_modus;
    int ud = est_deus ? c->deus.ultima_directio  : c->animus.ultima_directio;
    int up = est_deus ? c->deus.ultima_permissa   : c->animus.ultima_permissa;
    const char *audita = est_deus ? c->deus.audita : c->animus.audita;
    const char *mens   = est_deus ? c->deus.mens   : c->animus.mens;

    char *p = buf;
    size_t r = mag;
    int n;

    n = snprintf(p, r, "{\"vicinitas\": %s", vic);
    p += n; r -= (size_t)n;

    if (um > 0 && um <= 4 && ud >= 1 && ud <= 4) {
        n = snprintf(p, r, ", \"ultima_actio\": \"%s %s, %s\"",
                     modus_nomina[um], dir_nomina[ud],
                     up ? "successum" : "impeditus");
        p += n; r -= (size_t)n;
    }

    if (!est_deus && c->animus.satietas > 0) {
        n = snprintf(p, r, ", \"satietas\": %d", c->animus.satietas);
        p += n; r -= (size_t)n;
    }

    /* audita — verba audita */
    if (audita[0]) {
        n = snprintf(p, r, ", \"audita\": \"");
        p += n; r -= (size_t)n;
        for (const char *a = audita; *a && r > 4; a++) {
            if (*a == '"' || *a == '\\') { *p++ = '\\'; r--; }
            else if (*a == '\n') { *p++ = '\\'; *p++ = 'n'; r -= 2; continue; }
            *p++ = *a; r--;
        }
        *p++ = '"'; r--;
    }

    /* mens — cogitationes currentis */
    if (mens[0]) {
        n = snprintf(p, r, ", \"mens\": \"");
        p += n; r -= (size_t)n;
        for (const char *m = mens; *m && r > 4; m++) {
            if (*m == '"' || *m == '\\') { *p++ = '\\'; r--; }
            else if (*m == '\n') { *p++ = '\\'; *p++ = 'n'; r -= 2; continue; }
            *p++ = *m; r--;
        }
        *p++ = '"'; r--;
    }

    snprintf(p, r, "}");
}

/* mitte fasciculum asynchrone. reddit 1 si missum. */
static int mitte_pendentes(const tabula_t *tab, praecogitata_t *res,
                           int initium, int fm, int radius,
                           const char *sapientum,
                           const char *instructiones_completae)
{
    if (res->volantes_num >= VOLANTES_MAX) return 0;

    json_scriptor_t *js = json_scriptor_crea();
    if (!js) return 0;

    int vi = res->volantes_num;
    cogitatio_volans_t *vl = &res->volantes[vi];
    vl->num = fm;

    for (int i = 0; i < fm; i++) {
        cogitatio_pendens_t *p = &res->pendentes[initium + i];
        vl->xx[i] = p->x;
        vl->yy[i] = p->y;
        memcpy(vl->nomina[i], p->nomen, 8);

        char val[4096];
        aedifica_valorem(tab, p->x, p->y, radius, val, sizeof(val));
        json_scriptor_adde_crudum(js, p->nomen, val);
    }

    char *rogatum = json_scriptor_fini(js);
    if (!rogatum) return 0;

    int fossa = oraculum_mitte(sapientum, instructiones_completae, rogatum);
    free(rogatum);

    if (fossa < 0) return 0;

    vl->fossa = fossa;
    res->volantes_num++;
    return 1;
}

/* lege responsum completum, adde ad acta */
static void collige_responsum(const char *responsum,
                              const cogitatio_volans_t *vl,
                              praecogitata_t *res)
{
    const char *json_initium = strchr(responsum, '{');
    if (!json_initium) return;

    json_par_t pares[FASCICULUS_MAX];
    int np = json_lege(json_initium, pares, FASCICULUS_MAX);
    if (np <= 0) return;

    for (int i = 0; i < np; i++) {
        /* si clavis finitur ".mens", quaere actum iam additum */
        size_t clon = strlen(pares[i].clavis);
        if (clon > 5 &&
            strcmp(pares[i].clavis + clon - 5, ".mens") == 0) {
            char nomen[64];
            memcpy(nomen, pares[i].clavis, clon - 5);
            nomen[clon - 5] = '\0';
            for (int a = 0; a < res->num; a++) {
                if (res->acta[a].x < 0) continue;
                /* compara per nomen in volante */
                for (int j = 0; j < vl->num; j++) {
                    if (res->acta[a].x == vl->xx[j] &&
                        res->acta[a].y == vl->yy[j] &&
                        strcmp(nomen, vl->nomina[j]) == 0) {
                        snprintf(res->acta[a].actio.mens, MENS_MAX,
                                 "%s", pares[i].valor);
                        goto mens_inventa;
                    }
                }
            }
            mens_inventa:
            continue;
        }

        for (int j = 0; j < vl->num; j++) {
            if (strcmp(pares[i].clavis, vl->nomina[j]) != 0)
                continue;
            if (res->num >= PRAECOGITATA_MAX)
                return;
            res->acta[res->num].x = vl->xx[j];
            res->acta[res->num].y = vl->yy[j];
            res->acta[res->num].actio =
                parse_actionem(pares[i].valor);
            res->num++;
            break;
        }
    }
}

void cogitatio_praecogita(tabula_t *tab, genus_t genus,
                          int limen, int modulus,
                          int fasciculus_mag, int patientia,
                          int radius,
                          const char *sapientum,
                          const char *instructiones,
                          praecogitata_t *res)
{
    oraculum_processus();

    /* --- 1. collige responsa completa --- */
    {
        int dest = 0;
        for (int v = 0; v < res->volantes_num; v++) {
            if (res->volantes[v].fossa < 0) continue;

            char *responsum = NULL;
            int st = oraculum_status(res->volantes[v].fossa, &responsum);

            if (st == ORACULUM_PENDENS) {
                res->volantes[dest++] = res->volantes[v];
                continue;
            }

            if (st == ORACULUM_PARATUM && responsum)
                collige_responsum(responsum, &res->volantes[v], res);

            free(responsum);
            oraculum_dimitte(res->volantes[v].fossa);
        }
        res->volantes_num = dest;
    }

    /* --- 2. accumula cellulas quae cogitare volunt --- */
    for (int y = 0; y < tab->latus; y++) {
        for (int x = 0; x < tab->latus; x++) {
            const cella_t *c = tabula_da_const(tab, x, y);
            if (c->genus != genus)
                continue;
            if (rand() % modulus >= limen)
                continue;
            if (res->pendentes_num >= PRAECOGITATA_MAX)
                break;
            if (iam_in_processu(res, x, y))
                continue;

            cogitatio_pendens_t *p = &res->pendentes[res->pendentes_num];
            p->x = x;
            p->y = y;
            const char *nom = (genera_ops[genus].phylum == DEI)
                              ? c->deus.nomen : c->animus.nomen;
            memcpy(p->nomen, nom, 8);
            res->pendentes_num++;
        }
    }

    if (res->pendentes_num > 0)
        res->pendentes_gradus++;

    /* --- 3. mitte fasciculos quando parati --- */
    int paratus = (res->pendentes_num >= fasciculus_mag) ||
                  (res->pendentes_num > 0 &&
                   res->pendentes_gradus >= patientia);

    if (paratus) {
        char *inst = aedifica_instructiones(instructiones);
        if (!inst) return;

        int missi = 0;
        while (missi < res->pendentes_num) {
            if (res->volantes_num >= VOLANTES_MAX)
                break;
            int fm = res->pendentes_num - missi;
            if (fm > fasciculus_mag) fm = fasciculus_mag;

            if (!mitte_pendentes(tab, res, missi, fm, radius, sapientum, inst))
                break;
            missi += fm;
        }

        free(inst);

        if (missi > 0) {
            /* purga audita cellularum missarum */
            for (int i = 0; i < missi; i++) {
                cogitatio_pendens_t *p = &res->pendentes[i];
                cella_t *c = tabula_da(tab, p->x, p->y);
                if (genera_ops[c->genus].phylum == DEI)
                    c->deus.audita[0] = '\0';
                else
                    c->animus.audita[0] = '\0';
            }

            int reliqui = res->pendentes_num - missi;
            if (reliqui > 0)
                memmove(res->pendentes, res->pendentes + missi,
                        (size_t)reliqui * sizeof(res->pendentes[0]));
            res->pendentes_num = reliqui;
        }
        if (res->pendentes_num == 0)
            res->pendentes_gradus = 0;
    }
}

actio_t cogitatio_quaere(praecogitata_t *res, int x, int y)
{
    for (int i = 0; i < res->num; i++) {
        if (res->acta[i].x == x && res->acta[i].y == y) {
            actio_t act = res->acta[i].actio;
            res->acta[i] = res->acta[res->num - 1];
            res->num--;
            return act;
        }
    }
    return ACTIO_NIHIL;
}
