/*
 * json.c — implementatio JSON auxiliarium
 */

#include "json.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ================================================================
 * scriptor
 * ================================================================ */

struct json_scriptor {
    char  *data;
    size_t mag;     /* bytes scripti (sine NUL terminali) */
    size_t cap;     /* capacitas allocata */
    int    numerus; /* paria addita */
};

static int scriptor_cresc(json_scriptor_t *js, size_t opus)
{
    if (js->mag + opus + 1 <= js->cap)
        return 0;
    size_t nova = js->cap * 2;
    if (nova < js->mag + opus + 1)
        nova = js->mag + opus + 1;
    char *p = realloc(js->data, nova);
    if (!p) return -1;
    js->data = p;
    js->cap = nova;
    return 0;
}

static void scriptor_cat(json_scriptor_t *js, const char *s, size_t n)
{
    if (scriptor_cresc(js, n) < 0) return;
    memcpy(js->data + js->mag, s, n);
    js->mag += n;
    js->data[js->mag] = '\0';
}

static void scriptor_chordam(json_scriptor_t *js, const char *s)
{
    scriptor_cat(js, "\"", 1);
    for (const char *p = s; *p; p++) {
        char buf[8];
        size_t n;
        switch (*p) {
        case '"':  buf[0] = '\\'; buf[1] = '"';  n = 2; break;
        case '\\': buf[0] = '\\'; buf[1] = '\\'; n = 2; break;
        case '\n': buf[0] = '\\'; buf[1] = 'n';  n = 2; break;
        case '\r': buf[0] = '\\'; buf[1] = 'r';  n = 2; break;
        case '\t': buf[0] = '\\'; buf[1] = 't';  n = 2; break;
        default:
            if ((unsigned char)*p < 0x20) {
                n = (size_t)snprintf(buf, sizeof(buf),
                                     "\\u%04x", (unsigned char)*p);
            } else {
                buf[0] = *p;
                n = 1;
            }
        }
        scriptor_cat(js, buf, n);
    }
    scriptor_cat(js, "\"", 1);
}

json_scriptor_t *json_scriptor_crea(void)
{
    json_scriptor_t *js = calloc(1, sizeof(*js));
    if (!js) return NULL;
    js->cap = 256;
    js->data = malloc(js->cap);
    if (!js->data) { free(js); return NULL; }
    js->data[0] = '{';
    js->data[1] = '\0';
    js->mag = 1;
    return js;
}

void json_scriptor_adde(json_scriptor_t *js, const char *clavis,
                         const char *valor)
{
    if (!js) return;
    if (js->numerus > 0)
        scriptor_cat(js, ", ", 2);
    scriptor_chordam(js, clavis);
    scriptor_cat(js, ": ", 2);
    scriptor_chordam(js, valor);
    js->numerus++;
}

void json_scriptor_adde_crudum(json_scriptor_t *js, const char *clavis,
                               const char *valor)
{
    if (!js) return;
    if (js->numerus > 0)
        scriptor_cat(js, ", ", 2);
    scriptor_chordam(js, clavis);
    scriptor_cat(js, ": ", 2);
    scriptor_cat(js, valor, strlen(valor));
    js->numerus++;
}

char *json_scriptor_fini(json_scriptor_t *js)
{
    if (!js) return NULL;
    scriptor_cat(js, "}", 1);
    char *res = js->data;
    free(js);
    return res;
}

/* ================================================================
 * lector
 * ================================================================ */

/* declaratio anticipata */
static const char *nav_transili_valorem(const char *p);

static const char *transili_spatia(const char *p)
{
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    return p;
}

/*
 * lege chordam JSON (p ad '"' initialem).
 * scribit in buf[mag], reddit indicem post '"' terminalem.
 */
static const char *lege_chordam(const char *p, char *buf, size_t mag)
{
    if (*p != '"') return NULL;
    p++;

    size_t i = 0;
    while (*p && *p != '"') {
        char c;
        if (*p == '\\') {
            p++;
            if (!*p) return NULL;
            switch (*p) {
            case '"':  c = '"';  break;
            case '\\': c = '\\'; break;
            case '/':  c = '/';  break;
            case 'n':  c = '\n'; break;
            case 'r':  c = '\r'; break;
            case 't':  c = '\t'; break;
            case 'b':  c = '\b'; break;
            case 'f':  c = '\f'; break;
            case 'u':
                if (p[1] && p[2] && p[3] && p[4])
                    p += 4;
                c = '?';
                break;
            default: c = *p;
            }
        } else {
            c = *p;
        }
        if (i + 1 < mag)
            buf[i++] = c;
        p++;
    }
    buf[i] = '\0';

    if (*p == '"') p++;
    return p;
}

int json_lege(const char *json, json_par_t *pares, int max_pares)
{
    const char *p = transili_spatia(json);
    if (*p != '{') return -1;
    p++;

    int n = 0;
    while (n < max_pares) {
        p = transili_spatia(p);
        if (*p == '}' || *p == '\0')
            break;

        if (n > 0) {
            if (*p == ',') p++;
            p = transili_spatia(p);
        }

        if (*p != '"') break;

        /* clavis */
        char clavis[64];
        p = lege_chordam(p, clavis, sizeof(clavis));
        if (!p) return -1;

        p = transili_spatia(p);
        if (*p != ':') return -1;
        p++;
        p = transili_spatia(p);

        /* valor */
        char valor[64];
        if (*p == '"') {
            p = lege_chordam(p, valor, sizeof(valor));
            if (!p) return -1;
        } else if (*p == '{' || *p == '[') {
            /* transili objecta et indices */
            p = nav_transili_valorem(p);
            continue;
        } else {
            /* numeri, true, false, null */
            size_t i = 0;
            while (*p && *p != ',' && *p != '}' &&
                   *p != ' ' && *p != '\t' && *p != '\n' &&
                   i < sizeof(valor) - 1)
                valor[i++] = *p++;
            valor[i] = '\0';
        }

        memcpy(pares[n].clavis, clavis, sizeof(pares[n].clavis));
        memcpy(pares[n].valor, valor, sizeof(pares[n].valor));
        n++;
    }

    return n;
}

/* ================================================================
 * auxiliaria: effugere, extrahere, quaerere
 * ================================================================ */

char *json_effuge(const char *textus)
{
    size_t mag = 1;
    for (const char *p = textus; *p; p++) {
        switch (*p) {
        case '"': case '\\':            mag += 2; break;
        case '\n': case '\r': case '\t': mag += 2; break;
        default:
            if ((unsigned char)*p < 0x20) mag += 6;
            else                          mag += 1;
        }
    }

    char *res = malloc(mag);
    if (!res) return NULL;
    char *q = res;

    for (const char *p = textus; *p; p++) {
        switch (*p) {
        case '"':  *q++ = '\\'; *q++ = '"';  break;
        case '\\': *q++ = '\\'; *q++ = '\\'; break;
        case '\n': *q++ = '\\'; *q++ = 'n';  break;
        case '\r': *q++ = '\\'; *q++ = 'r';  break;
        case '\t': *q++ = '\\'; *q++ = 't';  break;
        default:
            if ((unsigned char)*p < 0x20)
                q += sprintf(q, "\\u%04x", (unsigned char)*p);
            else
                *q++ = *p;
        }
    }
    *q = '\0';
    return res;
}

/* ================================================================
 * navigator: proprius JSON parser cum recursivo descensu
 * ================================================================ */

/* transili chordam JSON (p ad '"' initialem). reddit post '"' terminalem. */
static const char *nav_transili_chordam(const char *p)
{
    if (*p != '"') return p;
    p++;
    while (*p) {
        if (*p == '\\') { p += 2; continue; }
        if (*p == '"')  { return p + 1; }
        p++;
    }
    return p;
}

/* transili quemlibet valorem JSON. reddit post valorem. */
static const char *nav_transili_valorem(const char *p)
{
    p = transili_spatia(p);
    switch (*p) {
    case '"':
        return nav_transili_chordam(p);
    case '{': {
        p++;
        p = transili_spatia(p);
        while (*p && *p != '}') {
            p = nav_transili_chordam(transili_spatia(p)); /* clavis */
            p = transili_spatia(p);
            if (*p == ':') p++;
            p = nav_transili_valorem(p);                  /* valor */
            p = transili_spatia(p);
            if (*p == ',') p = transili_spatia(p + 1);
        }
        if (*p == '}') p++;
        return p;
    }
    case '[': {
        p++;
        p = transili_spatia(p);
        while (*p && *p != ']') {
            p = nav_transili_valorem(p);
            p = transili_spatia(p);
            if (*p == ',') p = transili_spatia(p + 1);
        }
        if (*p == ']') p++;
        return p;
    }
    case 't': return p + 4; /* true */
    case 'f': return p + 5; /* false */
    case 'n': return p + 4; /* null */
    default: /* numerus */
        if (*p == '-') p++;
        while ((*p >= '0' && *p <= '9') || *p == '.' ||
               *p == 'e' || *p == 'E' || *p == '+' || *p == '-')
            p++;
        return p;
    }
}

/* quaere clavem in objecto. p ad '{'. reddit indicem ad valorem. */
static const char *nav_in_objecto(const char *p, const char *clavis,
                                  size_t clon)
{
    p = transili_spatia(p);
    if (*p != '{') return NULL;
    p = transili_spatia(p + 1);

    while (*p && *p != '}') {
        if (*p != '"') return NULL;
        /* compara clavem */
        const char *k = p + 1;
        const char *kfinis = k;
        while (*kfinis && *kfinis != '"') {
            if (*kfinis == '\\') kfinis++;
            kfinis++;
        }
        size_t klon = (size_t)(kfinis - k);
        p = kfinis + 1; /* post '"' */
        p = transili_spatia(p);
        if (*p == ':') p = transili_spatia(p + 1);

        if (klon == clon && memcmp(k, clavis, clon) == 0)
            return p; /* valor inventus */

        p = nav_transili_valorem(p);
        p = transili_spatia(p);
        if (*p == ',') p = transili_spatia(p + 1);
    }
    return NULL;
}

/* quaere indicem in array. p ad '['. reddit indicem ad valorem. */
static const char *nav_in_indice(const char *p, int index)
{
    p = transili_spatia(p);
    if (*p != '[') return NULL;
    p = transili_spatia(p + 1);

    for (int i = 0; i < index; i++) {
        if (*p == ']' || !*p) return NULL;
        p = nav_transili_valorem(p);
        p = transili_spatia(p);
        if (*p == ',') p = transili_spatia(p + 1);
    }

    return (*p && *p != ']') ? p : NULL;
}

/* naviga per viam punctatam: "a.b[0].c" */
static const char *json_naviga(const char *json, const char *via)
{
    const char *p = transili_spatia(json);

    while (*via) {
        if (*via == '.') { via++; continue; }

        if (*via == '[') {
            via++;
            int idx = 0;
            while (*via >= '0' && *via <= '9')
                idx = idx * 10 + (*via++ - '0');
            if (*via == ']') via++;
            p = nav_in_indice(p, idx);
        } else {
            const char *vfinis = via;
            while (*vfinis && *vfinis != '.' && *vfinis != '[')
                vfinis++;
            p = nav_in_objecto(p, via, (size_t)(vfinis - via));
            via = vfinis;
        }

        if (!p) return NULL;
    }

    return p;
}

/* extrahe chordam ab indice (p ad '"'). allocat, vocans liberet. */
static char *nav_extrahe_chordam(const char *p)
{
    if (*p != '"') return NULL;
    p++;

    size_t cap = 256, lon = 0;
    char *res = malloc(cap);
    if (!res) return NULL;

    while (*p && *p != '"') {
        char c;
        if (*p == '\\') {
            p++;
            if (!*p) break;
            switch (*p) {
            case '"':  c = '"';  break;
            case '\\': c = '\\'; break;
            case '/':  c = '/';  break;
            case 'n':  c = '\n'; break;
            case 'r':  c = '\r'; break;
            case 't':  c = '\t'; break;
            case 'b':  c = '\b'; break;
            case 'f':  c = '\f'; break;
            case 'u':
                if (p[1] && p[2] && p[3] && p[4]) p += 4;
                c = '?';
                break;
            default: c = *p;
            }
        } else {
            c = *p;
        }

        if (lon + 2 >= cap) {
            cap *= 2;
            char *novum = realloc(res, cap);
            if (!novum) { free(res); return NULL; }
            res = novum;
        }
        res[lon++] = c;
        p++;
    }
    res[lon] = '\0';
    return res;
}

char *json_da_chordam(const char *json, const char *via)
{
    const char *p = json_naviga(json, via);
    if (!p) return NULL;
    p = transili_spatia(p);
    return nav_extrahe_chordam(p);
}

long json_da_numerum(const char *json, const char *via)
{
    const char *p = json_naviga(json, via);
    if (!p) return 0;
    p = transili_spatia(p);
    return strtol(p, NULL, 10);
}

char *json_da_crudum(const char *json, const char *via)
{
    const char *p = json_naviga(json, via);
    if (!p) return NULL;
    p = transili_spatia(p);
    const char *finis = nav_transili_valorem(p);
    if (finis <= p) return NULL;
    size_t lon = (size_t)(finis - p);
    char *res = malloc(lon + 1);
    if (!res) return NULL;
    memcpy(res, p, lon);
    res[lon] = '\0';
    return res;
}

/* ================================================================
 * fasciculi
 * ================================================================ */

char *json_lege_fasciculum(const char *via)
{
    FILE *f = fopen(via, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long mag = ftell(f);
    if (mag < 0) { fclose(f); return NULL; }
    fseek(f, 0, SEEK_SET);
    char *buf = malloc((size_t)mag + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t lecta = fread(buf, 1, (size_t)mag, f);
    fclose(f);
    buf[lecta] = '\0';
    return buf;
}

/* ================================================================
 * JSONL
 * ================================================================ */

int json_pro_quaque_linea(const char *jsonl, json_linea_functor_t f, void *ctx)
{
    int n = 0;
    const char *p = jsonl;
    while (*p) {
        /* transili spatia et lineas vacuas */
        while (*p == '\n' || *p == '\r' || *p == ' ' || *p == '\t')
            p++;
        if (!*p) break;

        /* inveni finem lineae */
        const char *finis = p;
        while (*finis && *finis != '\n' && *finis != '\r')
            finis++;

        /* copia lineae in buffer temporarium */
        size_t lon = (size_t)(finis - p);
        char *linea = malloc(lon + 1);
        if (!linea) break;
        memcpy(linea, p, lon);
        linea[lon] = '\0';

        /* lege paria */
        json_par_t pares[32];
        int np = json_lege(linea, pares, 32);
        if (np > 0)
            f(pares, np, ctx);
        free(linea);
        n++;

        p = finis;
    }
    return n;
}

int json_claves(const char *json, char claves[][64], int max)
{
    const char *p = transili_spatia(json);
    if (*p != '{') return 0;
    p = transili_spatia(p + 1);

    int n = 0;
    while (*p && *p != '}' && n < max) {
        if (n > 0) {
            if (*p == ',') p = transili_spatia(p + 1);
        }
        if (*p != '"') break;
        char buf[64];
        p = lege_chordam(p, buf, sizeof(buf));
        if (!p) break;
        memcpy(claves[n], buf, 64);
        n++;
        p = transili_spatia(p);
        if (*p == ':') p = transili_spatia(p + 1);
        p = nav_transili_valorem(p);
        p = transili_spatia(p);
    }
    return n;
}

/* ================================================================
 * schema — lector et validator
 * ================================================================ */

int schema_lege(const char *json, schema_t *s)
{
    memset(s, 0, sizeof(*s));

    /* titulus */
    char *tit = json_da_chordam(json, "titulus");
    if (tit) {
        snprintf(s->titulus, sizeof(s->titulus), "%s", tit);
        free(tit);
    }

    /* proprietates — extrahe claves */
    char *prop_crudum = json_da_crudum(json, "properties");
    if (!prop_crudum)
        return -1;

    char nomina[SCHEMA_CAMPI_MAX][64];
    int nc = json_claves(prop_crudum, nomina, SCHEMA_CAMPI_MAX);

    for (int i = 0; i < nc && s->num_campi < SCHEMA_CAMPI_MAX; i++) {
        schema_campus_t *c = &s->campi[s->num_campi];
        snprintf(c->nomen, sizeof(c->nomen), "%s", nomina[i]);

        /* typus campi */
        char via[128];
        snprintf(via, sizeof(via), "properties.%s.type", nomina[i]);
        char *typ = json_da_chordam(json, via);
        if (typ) {
            if (strcmp(typ, "integer") == 0)
                c->typus = TYPUS_NUMERUS;
            else
                c->typus = TYPUS_CHORDA;
            free(typ);
        }

        s->num_campi++;
    }
    free(prop_crudum);

    /* required — lege indicem */
    char *req_crudum = json_da_crudum(json, "required");
    if (req_crudum && req_crudum[0] == '[') {
        for (int i = 0; i < s->num_campi; i++) {
            const char *p = req_crudum + 1;
            while (*p) {
                while (*p && (*p == ' ' || *p == ',' || *p == '\n'))
                    p++;
                if (*p == ']') break;
                if (*p == '"') {
                    const char *ini = p + 1;
                    const char *fin = strchr(ini, '"');
                    if (!fin) break;
                    size_t lon = (size_t)(fin - ini);
                    if (lon == strlen(s->campi[i].nomen) &&
                        memcmp(ini, s->campi[i].nomen, lon) == 0) {
                        s->campi[i].necessarium = 1;
                        break;
                    }
                    p = fin + 1;
                } else {
                    break;
                }
            }
        }
    }
    free(req_crudum);

    return 0;
}

int schema_lege_fasciculum(const char *via, schema_t *s)
{
    char *json = json_lege_fasciculum(via);
    if (!json) return -1;
    int res = schema_lege(json, s);
    free(json);
    return res;
}

/* est valor numerus integer? */
static int est_numerus(const char *v)
{
    if (!*v) return 0;
    const char *p = v;
    if (*p == '-') p++;
    if (!*p) return 0;
    while (*p) {
        if (!isdigit((unsigned char)*p))
            return 0;
        p++;
    }
    return 1;
}

int schema_valida(const schema_t *s, const json_par_t *pp, int n,
                  char *error, size_t mag)
{
    /* verifica campos necessarios */
    for (int i = 0; i < s->num_campi; i++) {
        if (!s->campi[i].necessarium)
            continue;
        int inventum = 0;
        for (int j = 0; j < n; j++) {
            if (strcmp(pp[j].clavis, s->campi[i].nomen) == 0) {
                inventum = 1;
                break;
            }
        }
        if (!inventum) {
            snprintf(error, mag, "campus necessarius deest: \"%s\"",
                     s->campi[i].nomen);
            return -1;
        }
    }

    /* verifica typum cuiusque campi in dato */
    for (int j = 0; j < n; j++) {
        const schema_campus_t *c = NULL;
        for (int i = 0; i < s->num_campi; i++) {
            if (strcmp(s->campi[i].nomen, pp[j].clavis) == 0) {
                c = &s->campi[i];
                break;
            }
        }

        if (!c) {
            snprintf(error, mag, "campus ignotus: \"%s\"", pp[j].clavis);
            return -1;
        }

        if (c->typus == TYPUS_NUMERUS && !est_numerus(pp[j].valor)) {
            snprintf(error, mag,
                     "campus \"%s\": expectatur numerus, datum \"%s\"",
                     c->nomen, pp[j].valor);
            return -1;
        }
    }

    return 0;
}

/* validator JSONL */

typedef struct {
    const schema_t *schema;
    int linea_num;
    int errores;
} validatio_ctx_t;

static void valida_lineam(const json_par_t *pp, int n, void *ctx)
{
    validatio_ctx_t *v = ctx;
    v->linea_num++;

    char error[256];
    if (schema_valida(v->schema, pp, n, error, sizeof(error)) < 0) {
        fprintf(stderr, "  linea %d: %s\n", v->linea_num, error);
        v->errores++;
    }
}

int schema_valida_jsonl(const schema_t *s, const char *jsonl)
{
    validatio_ctx_t ctx = { .schema = s, .linea_num = 0, .errores = 0 };
    json_pro_quaque_linea(jsonl, valida_lineam, &ctx);
    return ctx.errores;
}
