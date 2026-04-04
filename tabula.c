/*
 * tabula.c — implementatio tabulae toroidalis
 */

#include "tabula.h"
#include "utilia.h"
#include "ison.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* modulus semper positivus */
static int modulus(int a, int n)
{
    return ((a % n) + n) % n;
}

tabula_t *tabula_crea(const char *munda)
{
    /* lege tabula.ison */
    char via[256];
    snprintf(via, sizeof(via), "%s/tabula.ison", munda);
    char *ison = ison_lege_plicam(via);
    if (!ison) {
        fprintf(stderr, "error: %s legi non potuit\n", via);
        return NULL;
    }

    int latus = (int)ison_da_numerum(ison, "latus");
    free(ison);
    if (latus < 4)
        latus = 4;
    if (latus > 128)
        latus = 128;

    tabula_t *tab = malloc(sizeof(*tab));
    if (!tab)
        return NULL;

    tab->latus      = latus;
    tab->gradus_num = 0;
    tab->munda      = munda;
    tab->cellulae   = calloc((size_t)latus * latus, sizeof(cella_t));
    if (!tab->cellulae) {
        free(tab);
        return NULL;
    }
    return tab;
}

/* ================================================================
 * tabula_imple — popula tabulam ex plicis mundi
 * ================================================================ */

/* genera quae ex ISONL onerantur */
static const struct {
    genus_t genus;
    const char *phylum;
    const char *nomen;
} genera_isonl[] = {
    { SAXUM,   "fixa",   "saxum"   },
    { MURUS,   "fixa",   "murus"   },
    { RAPUM,   "cibi",   "rapum"   },
    { FUNGUS,  "cibi",   "fungus"  },
    { FELES,   "animae", "feles"   },
    { DALEKUS, "animae", "dalekus" },
    { URSUS,   "animae", "ursus"   },
    { CORVUS,  "animae", "corvus"  },
    { ZODUS,   "dei",    "zodus"   },
    { OCULUS,  "dei",    "oculus"  },
};
#define GENERA_ISONL_NUM (int)(sizeof(genera_isonl) / sizeof(genera_isonl[0]))

/* contextus pro ISONL iteratione */
typedef struct {
    tabula_t *tab;
    const char *tabula_ison;  /* ISON crudus pro positiones */
    genus_t genus;
} imple_ctx_t;

/* applica attributa individualia ex paribus ISON */
static void applica_attributa(
    cella_t *c, genus_t genus,
    const ison_par_t *pp, int n
) {
    c->pondus = par_da_int(pp, n, "pondus", c->pondus);

    phylum_t ph = genera_ops[genus].phylum;
    if (ph == ANIMA) {
        c->p.animus.vires     = par_da_int(pp, n, "vires", c->p.animus.vires);
        c->p.animus.vitalitas = par_da_int(pp, n, "vitalitas", c->p.animus.vitalitas);
        c->p.animus.satietas  = par_da_int(pp, n, "satietas", c->p.animus.satietas);
        const char *m         = par_da_chordam(pp, n, "mens", "");
        if (m[0])
            snprintf(c->p.animus.mens, MENS_MAX, "%s", m);
    } else if (ph == DEI) {
        c->p.deus.potentia = par_da_int(pp, n, "potentia", c->p.deus.potentia);
        const char *m      = par_da_chordam(pp, n, "mens", "");
        if (m[0])
            snprintf(c->p.deus.mens, MENS_MAX, "%s", m);
    }

    /* attributa propria generis */
    if (genus == FELES)
        c->g.feles.vagatio = par_da_int(pp, n, "vagatio", 0);
    else if (genus == DALEKUS)
        c->g.dalekus.energia = par_da_int(pp, n, "energia", 100);
    else if (genus == URSUS)
        c->g.ursus.ferocitas = par_da_int(pp, n, "ferocitas", 5);
    else if (genus == CORVUS)
        c->g.corvus.audacia = par_da_int(pp, n, "audacia", 3);
    else if (genus == OCULUS)
        c->g.oculus.visus_radius = par_da_int(pp, n, "visus_radius", 5);
    else if (genus == RAPUM || genus == FUNGUS)
        c->p.cibus.nutritio = par_da_int(pp, n, "nutritio", c->p.cibus.nutritio);
}

static void imple_lineam(const ison_par_t *pp, int n, void *ctx)
{
    imple_ctx_t *ic   = ctx;
    const char *nomen = par_da_chordam(pp, n, "nomen", "");
    if (!nomen[0])
        return;

    /* quaere positionem in tabula.ison */
    char via_x[128], via_y[128];
    snprintf(via_x, sizeof(via_x), "positiones.%s[0]", nomen);
    snprintf(via_y, sizeof(via_y), "positiones.%s[1]", nomen);
    int x = (int)ison_da_numerum(ic->tabula_ison, via_x);
    int y = (int)ison_da_numerum(ic->tabula_ison, via_y);

    /* pone cellam */
    tabula_pone(ic->tab, x, y, ic->genus);
    cella_t *c = tabula_da(ic->tab, x, y);

    /* scribe nomen */
    phylum_t ph = genera_ops[ic->genus].phylum;
    if (ph == ANIMA)
        snprintf(c->p.animus.nomen, sizeof(c->p.animus.nomen), "%s", nomen);
    else if (ph == DEI)
        snprintf(c->p.deus.nomen, sizeof(c->p.deus.nomen), "%s", nomen);

    /* applica attributa individualia */
    applica_attributa(c, ic->genus, pp, n);
}

void tabula_imple(tabula_t *tab)
{
    /* lege tabula.ison */
    char via[256];
    snprintf(via, sizeof(via), "%s/tabula.ison", tab->munda);
    char *tabula_ison = ison_lege_plicam(via);
    if (!tabula_ison)
        return;

    /* murus perimeter */
    char *murus_val = ison_da_chordam(tabula_ison, "murus");
    if (murus_val && strcmp(murus_val, "perimeter") == 0) {
        int l = tab->latus;
        for (int x = 0; x < l; x++) {
            tabula_pone(tab, x, 0, MURUS);
            tabula_pone(tab, x, l - 1, MURUS);
        }
        for (int y = 1; y < l - 1; y++) {
            tabula_pone(tab, 0, y, MURUS);
            tabula_pone(tab, l - 1, y, MURUS);
        }
    }
    free(murus_val);

    /* sapientum per genus */
    char *sap_crudum = ison_da_crudum(tabula_ison, "sapientum");
    if (sap_crudum) {
        for (int g = 0; g < GENERA_ISONL_NUM; g++) {
            char *val = ison_da_chordam(sap_crudum, genera_isonl[g].nomen);
            if (val) {
                snprintf(
                    tab->sapientum[genera_isonl[g].genus],
                    sizeof(tab->sapientum[0]), "%s", val
                );
                free(val);
            }
        }
        free(sap_crudum);
    }

    /* onera genera ex ISONL */
    for (int g = 0; g < GENERA_ISONL_NUM; g++) {
        char isonl_via[256];
        snprintf(
            isonl_via, sizeof(isonl_via), "%s/%s/%s.isonl",
            tab->munda, genera_isonl[g].phylum, genera_isonl[g].nomen
        );
        char *isonl = ison_lege_plicam(isonl_via);
        if (!isonl)
            continue;

        imple_ctx_t ctx = {
            .tab = tab,
            .tabula_ison = tabula_ison,
            .genus = genera_isonl[g].genus,
        };
        ison_pro_quaque_linea(isonl, imple_lineam, &ctx);
        free(isonl);
    }

    free(tabula_ison);
}

void tabula_libera(tabula_t *tab)
{
    if (tab) {
        free(tab->cellulae);
        free(tab);
    }
}

cella_t *tabula_da(tabula_t *tab, int x, int y)
{
    x = modulus(x, tab->latus);
    y = modulus(y, tab->latus);
    return &tab->cellulae[y * tab->latus + x];
}

const cella_t *tabula_da_const(const tabula_t *tab, int x, int y)
{
    x = modulus(x, tab->latus);
    y = modulus(y, tab->latus);
    return &tab->cellulae[y * tab->latus + x];
}

void tabula_vicinum(
    const tabula_t *tab, int x, int y, directio_t dir,
    int *vx, int *vy
) {
    /* delta pro quaque directione */
    static const int dx[] = { 0,  0,  0, -1,  1 };
    static const int dy[] = { 0, -1,  1,  0,  0 };
    /*                      NIH SEP MER OCC ORI */

    *vx = modulus(x + dx[dir], tab->latus);
    *vy = modulus(y + dy[dir], tab->latus);
}

void tabula_pone(tabula_t *tab, int x, int y, genus_t genus)
{
    cella_t *c = tabula_da(tab, x, y);
    c->genus   = genus;
    c->motum   = 0;
    if (genera_ops[genus].praepara)
        genera_ops[genus].praepara(c);
}

/* ================================================================
 * sermo — adde verba ad receptaculum auditorum animi
 * ================================================================ */

static void adde_sermonem(
    cella_t *vic, const char *ab_nomine,
    const char *sermo
) {
    if (!sermo[0])
        return;
    phylum_t ph = genera_ops[vic->genus].phylum;
    if (ph != ANIMA && ph != DEI)
        return;

    char *audita    = (ph == DEI) ? vic->p.deus.audita : vic->p.animus.audita;
    size_t cur      = strlen(audita);
    size_t reliquum = AUDITA_MAX - cur - 1;
    if (reliquum < 8)
        return;  /* receptaculum plenum */

    snprintf(audita + cur, reliquum, "[%s]: %s\n", ab_nomine, sermo);
}

static void clama_ad_vicinos(
    tabula_t *tab, int cx, int cy,
    const char *ab_nomine, const char *sermo
) {
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0)
                continue;
            cella_t *vic = tabula_da(tab, cx + dx, cy + dy);
            adde_sermonem(vic, ab_nomine, sermo);
        }
    }
}

/* ================================================================
 * pulsio — logica generica pondere fundata
 *
 * Ambula catenam a (sx,sy) in directione dir donec VACUUM
 * inveniatur. Summa ponderum catena. Si vires sufficiunt,
 * pelle totam catenam.
 * ================================================================ */

/* mete catenam pellendam: longitudo et summa ponderum.
 * reddit 1 si catena in vacuum terminatur, 0 si non. */
static int meti_catenam(
    tabula_t *tab, int sx, int sy, directio_t dir,
    int *catena_num, int *summa_pond
) {
    int latus = tab->latus;
    int n     = 0, sp = 0;
    int cx    = sx, cy = sy;
    while (n < latus) {
        cella_t *c = tabula_da(tab, cx, cy);
        if (c->genus == VACUUM)
            break;
        sp += c->pondus;
        n++;
        int nx, ny;
        tabula_vicinum(tab, cx, cy, dir, &nx, &ny);
        cx = nx;
        cy = ny;
    }
    *catena_num = n;
    *summa_pond = sp;
    return tabula_da(tab, cx, cy)->genus == VACUUM;
}

#define PROPOSITA_MAX 256

struct propositum {
    int pulsor_x, pulsor_y;
    directio_t dir;
    int catena_num;         /* cellulae pellendae (inter pulsorem et vacuum) */
    int validum;
};

/* an cellula (qx,qy) in via propositi sit */
static int propositum_continet(
    const tabula_t *tab,
    const struct propositum *p,
    int qx, int qy
) {
    int cx = p->pulsor_x, cy = p->pulsor_y;
    if (cx == qx && cy == qy)
        return 1;
    for (int i = 0; i < p->catena_num + 1; i++) {
        tabula_vicinum(tab, cx, cy, p->dir, &cx, &cy);
        if (cx == qx && cy == qy)
            return 1;
    }
    return 0;
}

/* an duo proposita vias communes habeant */
static int conflictum(
    const tabula_t *tab,
    const struct propositum *a,
    const struct propositum *b
) {
    int cx = b->pulsor_x, cy = b->pulsor_y;
    if (propositum_continet(tab, a, cx, cy))
        return 1;
    for (int i = 0; i < b->catena_num + 1; i++) {
        tabula_vicinum(tab, cx, cy, b->dir, &cx, &cy);
        if (propositum_continet(tab, a, cx, cy))
            return 1;
    }
    return 0;
}

/* exsequi unam pulsionem validam */
static void exsequi_pulsionem(tabula_t *tab, const struct propositum *p)
{
    int n = p->catena_num + 2; /* pulsor + catena + vacuum */
    int xs[130], ys[130];
    xs[0] = p->pulsor_x;
    ys[0] = p->pulsor_y;
    for (int k = 1; k < n; k++)
        tabula_vicinum(tab, xs[k-1], ys[k-1], p->dir, &xs[k], &ys[k]);

    /* permuta a fine ad initium */
    for (int k = n - 1; k > 0; k--) {
        cella_t *dst = tabula_da(tab, xs[k], ys[k]);
        cella_t *src = tabula_da(tab, xs[k-1], ys[k-1]);
        *dst         = *src;
        dst->motum   = 1;
    }

    /* munda originem */
    cella_t *orig = tabula_da(tab, xs[0], ys[0]);
    orig->genus   = VACUUM;
    orig->pondus  = 0;
    orig->motum   = 1;
    memset(&orig->p.animus, 0, sizeof(orig->p.animus));
}

/* parse coordinatas teleportationis ex sermo.
 * "X Y" = absolutae, "+DX +DY" = relativae ('+' detegit modum).
 * reddit 1 si parsum successum, 0 si defectus. */
static int parse_teleporta(
    const char *sermo, int cx, int cy, int latus,
    int *tx, int *ty
) {
    if (!sermo[0])
        return 0;

    int relativum = (strchr(sermo, '+') != NULL);

    int a, b;
    if (sscanf(sermo, "%d %d", &a, &b) != 2)
        return 0;

    if (relativum) {
        *tx = modulus(cx + a, latus);
        *ty = modulus(cy + b, latus);
    } else {
        *tx = modulus(a, latus);
        *ty = modulus(b, latus);
    }
    return 1;
}

/* move unum animum simpliciter (in vacuum) */
static void move_animum(tabula_t *tab, int x, int y, int vx, int vy)
{
    cella_t *c = tabula_da(tab, x, y);
    cella_t *vic = tabula_da(tab, vx, vy);
    cella_t movens = *c;
    *vic = movens;
    vic->motum = 1;
    c->genus = VACUUM;
    c->pondus = 0;
    c->motum = 1;
    memset(&c->p.animus, 0, sizeof(c->p.animus));
}

/* ================================================================
 * ursi praegradum — pulsio cum detectione conflictuum
 * ================================================================ */

static void ursi_praegradum(tabula_t *tab)
{
    struct propositum proposita[PROPOSITA_MAX];
    int prop_num = 0;
    int latus    = tab->latus;

    for (int y = 0; y < latus; y++) {
        for (int x = 0; x < latus; x++) {
            cella_t *c = tabula_da(tab, x, y);
            if (c->genus != URSUS || c->motum)
                continue;

            genus_ops_t *ops = &genera_ops[URSUS];
            if (!ops->cogito) {
                c->motum = 1;
                continue;
            }

            actio_t act = ops->cogito(tab, x, y);

            if (act.mens[0]) {
                memcpy(c->p.animus.mens, act.mens, MENS_MAX);
                c->p.animus.mens_gradus = tab->gradus_num;
            }

            if (act.modus == QUIESCE) {
                c->motum = 1;
                continue;
            }

            /* loquere / clama */
            if (act.modus == LOQUERE) {
                if (act.directio != DIR_NIHIL) {
                    int lx, ly;
                    tabula_vicinum(tab, x, y, act.directio, &lx, &ly);
                    adde_sermonem(
                        tabula_da(tab, lx, ly),
                        c->p.animus.nomen, act.sermo
                    );
                }
                c->p.animus.ultima_modus = LOQUERE;
                c->p.animus.ultima_directio = act.directio;
                c->p.animus.ultima_permissa = 1;
                c->motum = 1;
                continue;
            }
            if (act.modus == CLAMA) {
                clama_ad_vicinos(tab, x, y, c->p.animus.nomen, act.sermo);
                c->p.animus.ultima_modus = CLAMA;
                c->p.animus.ultima_directio = DIR_NIHIL;
                c->p.animus.ultima_permissa = 1;
                c->motum = 1;
                continue;
            }

            /* teleporta — coordinatis, non directione */
            if (act.modus == TELEPORTA) {
                c->p.animus.ultima_modus    = TELEPORTA;
                c->p.animus.ultima_directio = DIR_NIHIL;
                int tx, ty;
                if (parse_teleporta(act.sermo, x, y, latus, &tx, &ty)) {
                    cella_t *dest = tabula_da(tab, tx, ty);
                    if (genera_ops[dest->genus].phylum == DEI) {
                        c->p.animus.ultima_permissa = 0;
                        c->motum = 1;
                    } else if (
                        genera_ops[dest->genus].phylum == ANIMA &&
                        c->p.animus.vires > dest->p.animus.vitalitas
                    ) {
                        c->p.animus.ultima_permissa = 1;
                        move_animum(tab, x, y, tx, ty);
                    } else if (genera_ops[dest->genus].phylum == CIBUS) {
                        c->p.animus.satietas +=
                            (dest->genus == FUNGUS) ? 2 : 1;
                        c->p.animus.ultima_permissa = 1;
                        move_animum(tab, x, y, tx, ty);
                    } else if (dest->genus == VACUUM) {
                        c->p.animus.ultima_permissa = 1;
                        move_animum(tab, x, y, tx, ty);
                    } else {
                        c->p.animus.ultima_permissa = 0;
                        c->motum = 1;
                    }
                } else {
                    c->p.animus.ultima_permissa = 0;
                    c->motum = 1;
                }
                continue;
            }

            directio_t dir = act.directio;
            int vx, vy;
            tabula_vicinum(tab, x, y, dir, &vx, &vy);
            cella_t *vic = tabula_da(tab, vx, vy);

            c->p.animus.ultima_modus    = act.modus;
            c->p.animus.ultima_directio = dir;

            modus_t m = act.modus;
            if (m == PELLE   && vic->genus == VACUUM)
                m = MOVE;
            if (m == CAPE    && vic->genus == VACUUM)
                m = MOVE;
            if (m == OPPUGNA && vic->genus == VACUUM)
                m = MOVE;

            if (m == PELLE) {
                /* pelle catenam — pondere fundata */
                int cn, sp;
                if (
                    meti_catenam(tab, vx, vy, dir, &cn, &sp) &&
                    sp <= c->p.animus.vires &&
                    prop_num < PROPOSITA_MAX
                ) {
                    proposita[prop_num].pulsor_x   = x;
                    proposita[prop_num].pulsor_y   = y;
                    proposita[prop_num].dir        = dir;
                    proposita[prop_num].catena_num = cn;
                    proposita[prop_num].validum    = 1;
                    prop_num++;
                } else {
                    c->p.animus.ultima_permissa = 0;
                }
                c->motum = 1;
            } else if (m == CAPE && genera_ops[vic->genus].phylum == CIBUS) {
                /* cape: ede cibum et move */
                c->p.animus.satietas += (vic->genus == FUNGUS) ? 2 : 1;
                c->p.animus.ultima_permissa = 1;
                move_animum(tab, x, y, vx, vy);
            } else if (
                m == OPPUGNA &&
                genera_ops[vic->genus].phylum == ANIMA &&
                c->p.animus.vires > vic->p.animus.vitalitas
            ) {
                /* oppugna: neca animum et move in locum eius */
                c->p.animus.ultima_permissa = 1;
                move_animum(tab, x, y, vx, vy);
            } else if (m == TRAHE && vic->genus == VACUUM) {
                directio_t retro = (dir == SEPTENTRIO) ? MERIDIES :
                (dir == MERIDIES) ? SEPTENTRIO :
                (dir == OCCIDENS) ? ORIENS : OCCIDENS;
                int rx, ry;
                tabula_vicinum(tab, x, y, retro, &rx, &ry);
                cella_t *post = tabula_da(tab, rx, ry);
                int trahibile = (
                    post->genus != VACUUM &&
                    post->pondus <= c->p.animus.vires
                );
                move_animum(tab, x, y, vx, vy);
                if (trahibile) {
                    cella_t tractum = *post;
                    cella_t *orig = tabula_da(tab, x, y);
                    *orig = tractum;
                    orig->motum = 1;
                    post->genus = VACUUM;
                    post->pondus = 0;
                    post->motum = 1;
                    memset(&post->p.animus, 0, sizeof(post->p.animus));
                }
                c->p.animus.ultima_permissa = 1;
            } else if (vic->genus == VACUUM) {
                c->p.animus.ultima_permissa = 1;
                move_animum(tab, x, y, vx, vy);
            } else {
                c->p.animus.ultima_permissa = 0;
                c->motum = 1;
            }
        }
    }

    /* detege conflictus inter pulsiones */
    for (int i = 0; i < prop_num; i++) {
        if (!proposita[i].validum)
            continue;
        for (int j = i + 1; j < prop_num; j++) {
            if (!proposita[j].validum)
                continue;
            if (conflictum(tab, &proposita[i], &proposita[j])) {
                proposita[i].validum = 0;
                proposita[j].validum = 0;
            }
        }
    }

    /* exsequi pulsiones validas */
    for (int i = 0; i < prop_num; i++) {
        cella_t *urs = tabula_da(
            tab,
            proposita[i].pulsor_x, proposita[i].pulsor_y
        );
        if (proposita[i].validum) {
            urs->p.animus.ultima_permissa = 1;
            exsequi_pulsionem(tab, &proposita[i]);
        } else {
            urs->p.animus.ultima_permissa = 0;
        }
    }
}

/* ================================================================
 * dei praegradum — zodus, PELLE solum
 * ================================================================ */

static void dei_praegradum(tabula_t *tab)
{
    int latus = tab->latus;
    for (int y = 0; y < latus; y++) {
        for (int x = 0; x < latus; x++) {
            cella_t *c = tabula_da(tab, x, y);
            if (genera_ops[c->genus].phylum != DEI || c->motum)
                continue;

            genus_ops_t *ops = &genera_ops[c->genus];
            if (!ops->cogito) {
                c->motum = 1;
                continue;
            }

            actio_t act = ops->cogito(tab, x, y);

            if (act.mens[0]) {
                memcpy(c->p.deus.mens, act.mens, MENS_MAX);
                c->p.deus.mens_gradus = tab->gradus_num;
            }

            if (act.modus == QUIESCE) {
                c->motum = 1;
                continue;
            }

            /* loquere / clama */
            if (act.modus == LOQUERE) {
                if (act.directio != DIR_NIHIL) {
                    int lx, ly;
                    tabula_vicinum(tab, x, y, act.directio, &lx, &ly);
                    adde_sermonem(
                        tabula_da(tab, lx, ly),
                        c->p.deus.nomen, act.sermo
                    );
                }
                c->p.deus.ultima_modus = LOQUERE;
                c->p.deus.ultima_directio = act.directio;
                c->p.deus.ultima_permissa = 1;
                c->motum = 1;
                continue;
            }
            if (act.modus == CLAMA) {
                clama_ad_vicinos(tab, x, y, c->p.deus.nomen, act.sermo);
                c->p.deus.ultima_modus = CLAMA;
                c->p.deus.ultima_directio = DIR_NIHIL;
                c->p.deus.ultima_permissa = 1;
                c->motum = 1;
                continue;
            }

            /* teleporta — coordinatis, non directione */
            if (act.modus == TELEPORTA) {
                c->p.deus.ultima_modus    = TELEPORTA;
                c->p.deus.ultima_directio = DIR_NIHIL;
                int tx, ty;
                if (parse_teleporta(act.sermo, x, y, latus, &tx, &ty)) {
                    cella_t *dest = tabula_da(tab, tx, ty);
                    if (genera_ops[dest->genus].phylum == DEI) {
                        if (c->p.deus.potentia > dest->p.deus.potentia) {
                            c->p.deus.ultima_permissa = 1;
                            move_animum(tab, x, y, tx, ty);
                        } else {
                            c->p.deus.ultima_permissa = 0;
                            c->motum = 1;
                        }
                    } else {
                        c->p.deus.ultima_permissa = 1;
                        move_animum(tab, x, y, tx, ty);
                    }
                } else {
                    c->p.deus.ultima_permissa = 0;
                    c->motum = 1;
                }
                continue;
            }

            directio_t dir = act.directio;
            int vx, vy;
            tabula_vicinum(tab, x, y, dir, &vx, &vy);
            cella_t *vic = tabula_da(tab, vx, vy);

            c->p.deus.ultima_modus    = act.modus;
            c->p.deus.ultima_directio = dir;

            modus_t m = act.modus;
            if (m == PELLE   && vic->genus == VACUUM)
                m = MOVE;
            if (m == OPPUGNA && vic->genus == VACUUM)
                m = MOVE;

            if (m == PELLE) {
                /* dei pellit sine limite virium */
                int cn, sp;
                (void)sp;
                if (meti_catenam(tab, vx, vy, dir, &cn, &sp)) {
                    struct propositum p;
                    p.pulsor_x   = x;
                    p.pulsor_y   = y;
                    p.dir        = dir;
                    p.catena_num = cn;
                    p.validum    = 1;
                    exsequi_pulsionem(tab, &p);
                    c = tabula_da(tab, x, y);
                } else {
                    c->p.deus.ultima_permissa = 0;
                    c->motum = 1;
                }
            } else if (
                m == OPPUGNA &&
                genera_ops[vic->genus].phylum == ANIMA
            ) {
                /* dei oppugnat quemlibet animum */
                c->p.deus.ultima_permissa = 1;
                move_animum(tab, x, y, vx, vy);
            } else if (vic->genus == VACUUM) {
                c->p.deus.ultima_permissa = 1;
                move_animum(tab, x, y, vx, vy);
            } else {
                c->p.deus.ultima_permissa = 0;
                c->motum = 1;
            }
        }
    }
}

/* ================================================================ */

void tabula_gradus(tabula_t *tab)
{
    int latus = tab->latus;

    /* expurga vexilla motus */
    for (int i = 0; i < latus * latus; i++)
        tab->cellulae[i].motum = 0;

    /* dei agunt primissimo */
    dei_praegradum(tab);

    /* ursi agunt primo (pulsio cum detectione conflictuum) */
    ursi_praegradum(tab);

    /* ceteri cellulae cogitant et agunt (ANIMA tantum — DEI iam acti) */
    for (int y = 0; y < latus; y++) {
        for (int x = 0; x < latus; x++) {
            cella_t *c = tabula_da(tab, x, y);

            if (c->motum)
                continue;

            genus_ops_t *ops = &genera_ops[c->genus];
            if (ops->phylum == DEI)
                continue;
            if (!ops->cogito)
                continue;

            actio_t act = ops->cogito(tab, x, y);

            if (act.mens[0]) {
                memcpy(c->p.animus.mens, act.mens, MENS_MAX);
                c->p.animus.mens_gradus = tab->gradus_num;
            }

            if (act.modus == QUIESCE)
                continue;

            /* loquere / clama */
            if (act.modus == LOQUERE) {
                if (act.directio != DIR_NIHIL) {
                    int lx, ly;
                    tabula_vicinum(tab, x, y, act.directio, &lx, &ly);
                    adde_sermonem(
                        tabula_da(tab, lx, ly),
                        c->p.animus.nomen, act.sermo
                    );
                }
                c->p.animus.ultima_modus    = LOQUERE;
                c->p.animus.ultima_directio = act.directio;
                c->p.animus.ultima_permissa = 1;
                continue;
            }
            if (act.modus == CLAMA) {
                clama_ad_vicinos(tab, x, y, c->p.animus.nomen, act.sermo);
                c->p.animus.ultima_modus    = CLAMA;
                c->p.animus.ultima_directio = DIR_NIHIL;
                c->p.animus.ultima_permissa = 1;
                continue;
            }

            /* teleporta — coordinatis, non directione */
            if (act.modus == TELEPORTA) {
                c->p.animus.ultima_modus    = TELEPORTA;
                c->p.animus.ultima_directio = DIR_NIHIL;
                int tx, ty;
                if (parse_teleporta(act.sermo, x, y, latus, &tx, &ty)) {
                    cella_t *dest = tabula_da(tab, tx, ty);
                    if (genera_ops[dest->genus].phylum == DEI) {
                        c->p.animus.ultima_permissa = 0;
                    } else if (
                        genera_ops[dest->genus].phylum == ANIMA &&
                        c->p.animus.vires > dest->p.animus.vitalitas
                    ) {
                        c->p.animus.ultima_permissa = 1;
                        cella_t movens = *c;
                        *dest = movens;
                        dest->motum = 1;
                        c->genus = VACUUM;
                        c->pondus = 0;
                        c->motum = 1;
                        memset(&c->p.animus, 0, sizeof(c->p.animus));
                    } else if (genera_ops[dest->genus].phylum == CIBUS) {
                        c->p.animus.satietas +=
                            (dest->genus == FUNGUS) ? 2 : 1;
                        c->p.animus.ultima_permissa = 1;
                        cella_t movens = *c;
                        *dest = movens;
                        dest->motum = 1;
                        c->genus = VACUUM;
                        c->pondus = 0;
                        c->motum = 1;
                        memset(&c->p.animus, 0, sizeof(c->p.animus));
                    } else if (dest->genus == VACUUM) {
                        c->p.animus.ultima_permissa = 1;
                        cella_t movens = *c;
                        *dest = movens;
                        dest->motum = 1;
                        c->genus = VACUUM;
                        c->pondus = 0;
                        c->motum = 1;
                        memset(&c->p.animus, 0, sizeof(c->p.animus));
                    } else {
                        c->p.animus.ultima_permissa = 0;
                    }
                } else {
                    c->p.animus.ultima_permissa = 0;
                }
                continue;
            }

            directio_t dir = act.directio;
            int vx, vy;
            tabula_vicinum(tab, x, y, dir, &vx, &vy);

            cella_t *vic = tabula_da(tab, vx, vy);

            c->p.animus.ultima_modus    = act.modus;
            c->p.animus.ultima_directio = dir;

            modus_t m = act.modus;
            if (m == CAPE && vic->genus == VACUUM)
                m = MOVE;
            if (m == OPPUGNA && vic->genus == VACUUM)
                m = MOVE;

            int permissum = 0;

            if (m == MOVE && vic->genus == VACUUM)
                permissum = 1;
            else if (m == CAPE && genera_ops[vic->genus].phylum == CIBUS) {
                c->p.animus.satietas += (vic->genus == FUNGUS) ? 2 : 1;
                permissum = 1;
            } else if (
                m == OPPUGNA &&
                genera_ops[vic->genus].phylum == ANIMA &&
                c->p.animus.vires > vic->p.animus.vitalitas
            ) {
                permissum = 1;
            }

            if (permissum) {
                c->p.animus.ultima_permissa = 1;
                cella_t movens = *c;
                *vic = movens;
                vic->motum = 1;
                c->genus = VACUUM;
                c->pondus = 0;
                c->motum = 1;
                memset(&c->p.animus, 0, sizeof(c->p.animus));
            } else {
                c->p.animus.ultima_permissa = 0;
            }
        }
    }

    tab->gradus_num++;
}
