/*
 * numerus.c — arithmetica numeri magni, curva elliptica P-256,
 *             verificatio RSA, resolutio ASN.1/X.509, alea
 *
 * Numerus magnus: tabulatum verborum uint32_t, ordo minoris ponderis.
 *   v[0] = verbum minimi ponderis (least significant)
 *
 * Sine dependentiis externis.
 */

#include "arcana.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

/* --- alea --- */

int alea_imple(uint8_t *alveus, size_t mag)
{
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) return -1;
    size_t lectum = 0;
    while (lectum < mag) {
        ssize_t r = read(fd, alveus + lectum, mag - lectum);
        if (r <= 0) { close(fd); return -1; }
        lectum += (size_t)r;
    }
    close(fd);
    return 0;
}

/* --- numerus magnus --- */

void nm_ex_nihilo(nm_t *a)
{
    memset(a->v, 0, sizeof(a->v));
    a->n = 1;
}

static void nm_normaliza(nm_t *a)
{
    while (a->n > 1 && a->v[a->n - 1] == 0)
        a->n--;
}

/* ex octis big-endian */
void nm_ex_octis(nm_t *a, const uint8_t *data, size_t mag)
{
    nm_ex_nihilo(a);
    if (mag == 0) return;

    /* praetermitte nulos ducentes */
    while (mag > 0 && *data == 0) { data++; mag--; }
    if (mag == 0) return;

    int nv = (int)((mag + 3) / 4);
    if (nv > NM_VERBA) nv = NM_VERBA;
    a->n = nv;

    for (size_t i = 0; i < mag && (int)((mag - 1 - i) / 4) < NM_VERBA; i++) {
        int vi = (int)(mag - 1 - i) / 4;
        int bi = (int)(mag - 1 - i) % 4;
        a->v[vi] |= (uint32_t)data[i] << (bi * 8);
    }
    nm_normaliza(a);
}

/* ad octos big-endian, complens cum nulis */
void nm_ad_octos(const nm_t *a, uint8_t *data, size_t mag)
{
    memset(data, 0, mag);
    for (int i = 0; i < a->n; i++) {
        uint32_t w = a->v[i];
        for (int b = 0; b < 4; b++) {
            size_t pos = mag - 1 - (size_t)(i * 4 + b);
            if (pos < mag)
                data[pos] = (uint8_t)(w >> (b * 8));
        }
    }
}

int nm_compara(const nm_t *a, const nm_t *b)
{
    int m = a->n > b->n ? a->n : b->n;
    for (int i = m - 1; i >= 0; i--) {
        uint32_t av = (i < a->n) ? a->v[i] : 0;
        uint32_t bv = (i < b->n) ? b->v[i] : 0;
        if (av > bv) return 1;
        if (av < bv) return -1;
    }
    return 0;
}

static int nm_est_nihil(const nm_t *a)
{
    return a->n == 1 && a->v[0] == 0;
}

static int nm_bitus(const nm_t *a, int i)
{
    int vi = i / 32, bi = i % 32;
    if (vi >= a->n) return 0;
    return (a->v[vi] >> bi) & 1;
}

static int nm_summa_bitorum(const nm_t *a)
{
    if (nm_est_nihil(a)) return 0;
    int bits = (a->n - 1) * 32;
    uint32_t w = a->v[a->n - 1];
    while (w) { bits++; w >>= 1; }
    return bits;
}

void nm_adde(nm_t *r, const nm_t *a, const nm_t *b)
{
    uint64_t portatio = 0;
    int m = a->n > b->n ? a->n : b->n;
    int i;
    for (i = 0; i < m || portatio; i++) {
        uint64_t summa = portatio;
        if (i < a->n) summa += a->v[i];
        if (i < b->n) summa += b->v[i];
        if (i < NM_VERBA)
            r->v[i] = (uint32_t)summa;
        portatio = summa >> 32;
    }
    /* nula verba superstantia */
    for (; i < NM_VERBA; i++) r->v[i] = 0;
    r->n = m + 1;
    if (r->n > NM_VERBA) r->n = NM_VERBA;
    nm_normaliza(r);
}

/* r = a - b (praesumit a >= b) */
void nm_subtrahe(nm_t *r, const nm_t *a, const nm_t *b)
{
    int64_t mutuum = 0;
    int m = a->n > b->n ? a->n : b->n;
    int i;
    for (i = 0; i < m; i++) {
        int64_t diff = mutuum;
        if (i < a->n) diff += (int64_t)a->v[i];
        if (i < b->n) diff -= (int64_t)b->v[i];
        if (i < NM_VERBA)
            r->v[i] = (uint32_t)(diff & 0xFFFFFFFF);
        mutuum = diff >> 32;
    }
    for (; i < NM_VERBA; i++) r->v[i] = 0;
    r->n = m;
    nm_normaliza(r);
}

void nm_multiplica(nm_t *r, const nm_t *a, const nm_t *b)
{
    nm_t temp;
    nm_ex_nihilo(&temp);
    temp.n = a->n + b->n;
    if (temp.n > NM_VERBA) temp.n = NM_VERBA;

    for (int i = 0; i < a->n; i++) {
        uint64_t portatio = 0;
        for (int j = 0; j < b->n && i + j < NM_VERBA; j++) {
            uint64_t prod = (uint64_t)a->v[i] * b->v[j] +
                            temp.v[i + j] + portatio;
            temp.v[i + j] = (uint32_t)prod;
            portatio = prod >> 32;
        }
        if (i + b->n < NM_VERBA)
            temp.v[i + b->n] += (uint32_t)portatio;
    }
    nm_normaliza(&temp);
    *r = temp;
}

/* divisio Knuthiana: q = a / b, rem = a % b */
void nm_divide(nm_t *q, nm_t *rem, const nm_t *a, const nm_t *b)
{
    if (nm_est_nihil(b)) { nm_ex_nihilo(q); nm_ex_nihilo(rem); return; }
    if (nm_compara(a, b) < 0) {
        nm_ex_nihilo(q);
        *rem = *a;
        return;
    }

    /* divisio bitus-per-bitum */
    nm_ex_nihilo(q);
    nm_ex_nihilo(rem);

    int nbits = nm_summa_bitorum(a);
    for (int i = nbits - 1; i >= 0; i--) {
        /* rem = rem * 2 */
        uint32_t c = 0;
        int nn = rem->n;
        for (int j = 0; j <= nn && j < NM_VERBA; j++) {
            uint32_t novum = (rem->v[j] << 1) | c;
            c = rem->v[j] >> 31;
            rem->v[j] = novum;
        }
        /* auge n si portatio excessit */
        while (rem->n < NM_VERBA && rem->v[rem->n] != 0)
            rem->n++;

        /* inserere bitum */
        rem->v[0] |= (uint32_t)nm_bitus(a, i);
        nm_normaliza(rem);

        if (nm_compara(rem, b) >= 0) {
            nm_subtrahe(rem, rem, b);
            /* pone bitum in q */
            int vi = i / 32;
            if (vi < NM_VERBA) {
                q->v[vi] |= (uint32_t)1 << (i % 32);
                if (vi >= q->n) q->n = vi + 1;
            }
        }
    }
    nm_normaliza(q);
    nm_normaliza(rem);
}

void nm_modulo(nm_t *r, const nm_t *a, const nm_t *m)
{
    nm_t q;
    nm_divide(&q, r, a, m);
}

void nm_modmul(nm_t *r, const nm_t *a, const nm_t *b, const nm_t *m)
{
    nm_t prod;
    nm_multiplica(&prod, a, b);
    nm_modulo(r, &prod, m);
}

/* --- Montgomery --- */

typedef struct {
    nm_t modulus;
    int k;              /* verba in modulo */
    uint32_t m_inv;     /* -m^(-1) mod 2^32 */
    nm_t r_quadratum;   /* R^2 mod m, ubi R = 2^(k*32) */
} mont_t;

/* computa -m^(-1) mod 2^32 per iterationem Newtoni */
static uint32_t mont_inv(uint32_t m0)
{
    uint32_t x = 1;
    for (int i = 0; i < 5; i++)
        x = x * (2 - m0 * x);
    return (uint32_t)(-(int32_t)x);
}

static void mont_initia(mont_t *mt, const nm_t *m)
{
    mt->modulus = *m;
    mt->k = m->n;
    mt->m_inv = mont_inv(m->v[0]);

    /* R^2 mod m: R = 2^(k*32), computamus R mod m, deinde R^2 mod m */
    /* methodus: initia r = 1, dupla k*64 vices cum reductione */
    nm_t r;
    nm_ex_nihilo(&r);
    r.v[0] = 1;
    for (int i = 0; i < mt->k * 64; i++) {
        nm_adde(&r, &r, &r);
        if (nm_compara(&r, m) >= 0)
            nm_subtrahe(&r, &r, m);
    }
    mt->r_quadratum = r;
}

/* REDC: T * R^(-1) mod m */
static void mont_redc(const mont_t *mt, nm_t *T)
{
    int k = mt->k;
    for (int i = 0; i < k; i++) {
        uint32_t u = T->v[i] * mt->m_inv;
        /* T += u * m * 2^(32*i) */
        uint64_t portatio = 0;
        for (int j = 0; j < k; j++) {
            int idx = i + j;
            if (idx >= NM_VERBA) break;
            uint64_t prod = (uint64_t)u * mt->modulus.v[j]
                            + T->v[idx] + portatio;
            T->v[idx] = (uint32_t)prod;
            portatio = prod >> 32;
        }
        for (int j = i + k; j < NM_VERBA && portatio; j++) {
            uint64_t s = (uint64_t)T->v[j] + portatio;
            T->v[j] = (uint32_t)s;
            portatio = s >> 32;
        }
    }
    /* T >>= k*32 */
    for (int i = 0; i < NM_VERBA - k; i++)
        T->v[i] = T->v[i + k];
    for (int i = NM_VERBA - k; i < NM_VERBA; i++)
        T->v[i] = 0;
    T->n = k + 1;
    if (T->n > NM_VERBA) T->n = NM_VERBA;
    nm_normaliza(T);

    if (nm_compara(T, &mt->modulus) >= 0)
        nm_subtrahe(T, T, &mt->modulus);
}

/* a → ā = a * R mod m */
static void mont_in(const mont_t *mt, nm_t *ar, const nm_t *a)
{
    nm_t prod;
    nm_multiplica(&prod, a, &mt->r_quadratum);
    *ar = prod;
    mont_redc(mt, ar);
}

/* ā → a = ā * R^(-1) mod m */
static void mont_ex(const mont_t *mt, nm_t *a, const nm_t *ar)
{
    *a = *ar;
    mont_redc(mt, a);
}

/* ā * b̄ * R^(-1) mod m */
static void mont_mul(const mont_t *mt, nm_t *r,
                     const nm_t *a, const nm_t *b)
{
    nm_t prod;
    nm_multiplica(&prod, a, b);
    *r = prod;
    mont_redc(mt, r);
}

void nm_modpot(nm_t *r, const nm_t *basis, const nm_t *exponens,
               const nm_t *modulus)
{
    /* moduli parvi vel pares: recidunt ad methodum veterem */
    if (modulus->n < 2 || !(modulus->v[0] & 1)) {
        nm_t base_mod;
        nm_modulo(&base_mod, basis, modulus);
        nm_ex_nihilo(r);
        r->v[0] = 1;
        int nbits = nm_summa_bitorum(exponens);
        for (int i = nbits - 1; i >= 0; i--) {
            nm_modmul(r, r, r, modulus);
            if (nm_bitus(exponens, i))
                nm_modmul(r, r, &base_mod, modulus);
        }
        return;
    }

    mont_t mt;
    mont_initia(&mt, modulus);

    /* converte in formam Montgomery */
    nm_t base_mont, acc_mont;
    nm_t base_mod;
    nm_modulo(&base_mod, basis, modulus);
    mont_in(&mt, &base_mont, &base_mod);

    /* acc = 1 in Montgomery = R mod m */
    nm_t unum;
    nm_ex_nihilo(&unum);
    unum.v[0] = 1;
    mont_in(&mt, &acc_mont, &unum);

    int nbits = nm_summa_bitorum(exponens);
    for (int i = nbits - 1; i >= 0; i--) {
        mont_mul(&mt, &acc_mont, &acc_mont, &acc_mont);
        if (nm_bitus(exponens, i))
            mont_mul(&mt, &acc_mont, &acc_mont, &base_mont);
    }

    /* converte retro */
    mont_ex(&mt, r, &acc_mont);
}

/* --- curva elliptica P-256 --- */

/* primus campi: p = 2^256 - 2^224 + 2^192 + 2^96 - 1 */
const nm_t EC_PRIMUS = {
    .v = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
           0x00000000, 0x00000000, 0x00000001, 0xFFFFFFFF },
    .n = 8
};

/* ordo: n */
const nm_t EC_ORDO = {
    .v = { 0xFC632551, 0xF3B9CAC2, 0xA7179E84, 0xBCE6FAAD,
           0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF },
    .n = 8
};

/* generator G */
const ec_punctum_t EC_GENERATOR = {
    .x = {
        .v = { 0xD898C296, 0xF4A13945, 0x2DEB33A0, 0x77037D81,
               0x63A440F2, 0xF8BCE6E5, 0xE12C4247, 0x6B17D1F2 },
        .n = 8
    },
    .y = {
        .v = { 0x37BF51F5, 0xCBB64068, 0x6B315ECE, 0x2BCE3357,
               0x7C0F9E16, 0x8EE7EB4A, 0xFE1A7F9B, 0x4FE342E2 },
        .n = 8
    },
    .infinitum = 0
};

/* a = -3 mod p */
static const nm_t EC_A = {
    .v = { 0xFFFFFFFC, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
           0x00000000, 0x00000000, 0x00000001, 0xFFFFFFFF },
    .n = 8
};

/* --- arithmetica campi Fp --- */

static void fp_adde(nm_t *r, const nm_t *a, const nm_t *b)
{
    nm_adde(r, a, b);
    if (nm_compara(r, &EC_PRIMUS) >= 0)
        nm_subtrahe(r, r, &EC_PRIMUS);
}

static void fp_subtrahe(nm_t *r, const nm_t *a, const nm_t *b)
{
    if (nm_compara(a, b) >= 0) {
        nm_subtrahe(r, a, b);
    } else {
        nm_t temp;
        nm_adde(&temp, a, &EC_PRIMUS);
        nm_subtrahe(r, &temp, b);
    }
}

static void fp_multiplica(nm_t *r, const nm_t *a, const nm_t *b)
{
    nm_modmul(r, a, b, &EC_PRIMUS);
}

/* inversio per theorema Fermati: a^(-1) = a^(p-2) mod p */
static void fp_inversa(nm_t *r, const nm_t *a)
{
    nm_t exp;
    nm_t duo;
    nm_ex_nihilo(&duo);
    duo.v[0] = 2;
    nm_subtrahe(&exp, &EC_PRIMUS, &duo);
    nm_modpot(r, a, &exp, &EC_PRIMUS);
}

/* --- operationes puncti --- */

void ec_adde(ec_punctum_t *r, const ec_punctum_t *P, const ec_punctum_t *Q)
{
    if (P->infinitum) { *r = *Q; return; }
    if (Q->infinitum) { *r = *P; return; }

    /* si P == -Q, reddit infinitum */
    nm_t summa_y;
    fp_adde(&summa_y, &P->y, &Q->y);
    if (nm_compara(&P->x, &Q->x) == 0 && nm_est_nihil(&summa_y)) {
        nm_ex_nihilo(&r->x);
        nm_ex_nihilo(&r->y);
        r->infinitum = 1;
        return;
    }

    nm_t lambda, temp, temp2;

    if (nm_compara(&P->x, &Q->x) == 0 &&
        nm_compara(&P->y, &Q->y) == 0) {
        /* duplicatio: lambda = (3*x^2 + a) / (2*y) */
        nm_t x2;
        fp_multiplica(&x2, &P->x, &P->x);         /* x^2 */
        fp_adde(&temp, &x2, &x2);
        fp_adde(&temp, &temp, &x2);                 /* 3*x^2 */
        fp_adde(&temp, &temp, &EC_A);               /* 3*x^2 + a */

        nm_t y2;
        fp_adde(&y2, &P->y, &P->y);                 /* 2*y */
        fp_inversa(&temp2, &y2);
        fp_multiplica(&lambda, &temp, &temp2);
    } else {
        /* additio: lambda = (y2 - y1) / (x2 - x1) */
        fp_subtrahe(&temp, &Q->y, &P->y);
        fp_subtrahe(&temp2, &Q->x, &P->x);
        nm_t inv;
        fp_inversa(&inv, &temp2);
        fp_multiplica(&lambda, &temp, &inv);
    }

    /* x3 = lambda^2 - x1 - x2 */
    nm_t l2;
    fp_multiplica(&l2, &lambda, &lambda);
    fp_subtrahe(&temp, &l2, &P->x);
    fp_subtrahe(&r->x, &temp, &Q->x);

    /* y3 = lambda * (x1 - x3) - y1 */
    fp_subtrahe(&temp, &P->x, &r->x);
    fp_multiplica(&temp2, &lambda, &temp);
    fp_subtrahe(&r->y, &temp2, &P->y);

    r->infinitum = 0;
}

/* multiplicatio scalaris: R = k * P (methodus duplica-et-adde) */
void ec_multiplica(ec_punctum_t *r, const nm_t *k, const ec_punctum_t *P)
{
    ec_punctum_t acc;
    nm_ex_nihilo(&acc.x);
    nm_ex_nihilo(&acc.y);
    acc.infinitum = 1;

    int nbits = nm_summa_bitorum(k);
    for (int i = nbits - 1; i >= 0; i--) {
        ec_punctum_t duplex;
        ec_adde(&duplex, &acc, &acc);
        if (nm_bitus(k, i)) {
            ec_adde(&acc, &duplex, P);
        } else {
            acc = duplex;
        }
    }
    *r = acc;
}

/* --- RSA --- */

/* DigestInfo pro SHA-256 (DER) */
static const uint8_t DIGESTINFO_SHA256[] = {
    0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86,
    0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x05,
    0x00, 0x04, 0x20
};
#define DIGESTINFO_MAG 19

int rsa_verifica(const rsa_clavis_t *clavis,
                 const uint8_t *signatura, size_t sig_mag,
                 const uint8_t digestum[32])
{
    nm_t s, n, e, m;
    nm_ex_octis(&s, signatura, sig_mag);
    nm_ex_octis(&n, clavis->modulus, clavis->modulus_mag);
    nm_ex_octis(&e, clavis->exponens, clavis->exponens_mag);

    /* m = s^e mod n */
    nm_modpot(&m, &s, &e, &n);

    /* converte ad octos */
    uint8_t *effectus = malloc(clavis->modulus_mag);
    if (!effectus) return -1;
    nm_ad_octos(&m, effectus, clavis->modulus_mag);

    /* verifica PKCS#1 v1.5: 00 01 FF...FF 00 DigestInfo Hash */
    size_t mag = clavis->modulus_mag;
    int exitus = -1;

    if (mag >= DIGESTINFO_MAG + 32 + 11 &&
        effectus[0] == 0x00 && effectus[1] == 0x01) {
        size_t i = 2;
        while (i < mag && effectus[i] == 0xFF) i++;
        if (i >= 10 && i < mag && effectus[i] == 0x00) {
            i++;
            if (i + DIGESTINFO_MAG + 32 == mag &&
                memcmp(effectus + i, DIGESTINFO_SHA256, DIGESTINFO_MAG) == 0 &&
                memcmp(effectus + i + DIGESTINFO_MAG, digestum, 32) == 0) {
                exitus = 0;
            }
        }
    }
    free(effectus);
    return exitus;
}

/* RSA occultatio PKCS#1 v1.5 (pro clave publica) */
int rsa_occulta_pkcs1(const rsa_clavis_t *clavis,
                      const uint8_t *nuntius, size_t nun_mag,
                      uint8_t *occultus, size_t *occ_mag)
{
    size_t k = clavis->modulus_mag;
    if (nun_mag > k - 11) return -1;

    uint8_t *em = malloc(k);
    if (!em) return -1;
    em[0] = 0x00;
    em[1] = 0x02;

    /* complimentum aleatorium (non nulum) */
    size_t ps_mag = k - nun_mag - 3;
    if (alea_imple(em + 2, ps_mag) < 0) { free(em); return -1; }
    for (size_t i = 0; i < ps_mag; i++)
        if (em[2 + i] == 0) em[2 + i] = 0x42;
    em[2 + ps_mag] = 0x00;
    memcpy(em + 3 + ps_mag, nuntius, nun_mag);

    nm_t m_val, n, e, c;
    nm_ex_octis(&m_val, em, k);
    nm_ex_octis(&n, clavis->modulus, clavis->modulus_mag);
    nm_ex_octis(&e, clavis->exponens, clavis->exponens_mag);
    nm_modpot(&c, &m_val, &e, &n);
    nm_ad_octos(&c, occultus, k);
    *occ_mag = k;

    free(em);
    return 0;
}

/* --- ASN.1 / X.509 --- */

/* lege caput ASN.1 DER (tag + longitudo) */
static int asn1_caput(const uint8_t **p, const uint8_t *finis,
                      uint8_t *signum, size_t *longitudo)
{
    if (*p >= finis) return -1;
    *signum = *(*p)++;
    if (*p >= finis) return -1;
    uint8_t prim = *(*p)++;
    if (prim < 0x80) {
        *longitudo = prim;
    } else {
        int nb = prim & 0x7f;
        if (nb > 4 || *p + nb > finis) return -1;
        *longitudo = 0;
        for (int i = 0; i < nb; i++)
            *longitudo = (*longitudo << 8) | *(*p)++;
    }
    return 0;
}

/* praetermitte elementum ASN.1 */
static int asn1_praetermitte(const uint8_t **p, const uint8_t *finis)
{
    uint8_t signum;
    size_t longitudo;
    if (asn1_caput(p, finis, &signum, &longitudo) < 0) return -1;
    if (*p + longitudo > finis) return -1;
    *p += longitudo;
    return 0;
}

int asn1_extrahe_rsa(const uint8_t *cert, size_t mag, rsa_clavis_t *clavis)
{
    const uint8_t *p = cert;
    const uint8_t *finis = cert + mag;
    uint8_t signum;
    size_t longitudo;

    /* SEQUENCE exterior (Certificate) */
    if (asn1_caput(&p, finis, &signum, &longitudo) < 0 || signum != 0x30)
        return -1;

    /* TBSCertificate SEQUENCE */
    if (asn1_caput(&p, finis, &signum, &longitudo) < 0 || signum != 0x30)
        return -1;
    const uint8_t *tbs_finis = p + longitudo;

    /* versio [0] EXPLICIT (optionalis) */
    const uint8_t *temp = p;
    if (asn1_caput(&temp, tbs_finis, &signum, &longitudo) < 0)
        return -1;
    if ((signum & 0xe0) == 0xa0) {
        p = temp + longitudo;
    }

    /* praetermitte: numerusSerialis, algorithmus, emittens, validitas, subiectum */
    for (int i = 0; i < 5; i++) {
        if (asn1_praetermitte(&p, tbs_finis) < 0)
            return -1;
    }

    /* SubjectPublicKeyInfo SEQUENCE */
    if (asn1_caput(&p, tbs_finis, &signum, &longitudo) < 0 || signum != 0x30)
        return -1;
    const uint8_t *spki_finis = p + longitudo;

    /* AlgorithmIdentifier SEQUENCE — praetermitte */
    if (asn1_praetermitte(&p, spki_finis) < 0)
        return -1;

    /* subjectPublicKey BIT STRING */
    if (asn1_caput(&p, spki_finis, &signum, &longitudo) < 0 || signum != 0x03)
        return -1;
    if (longitudo < 1) return -1;
    p++;
    longitudo--;

    /* RSAPublicKey SEQUENCE interna */
    const uint8_t *rsa_finis = p + longitudo;
    if (asn1_caput(&p, rsa_finis, &signum, &longitudo) < 0 || signum != 0x30)
        return -1;

    /* modulus INTEGER */
    if (asn1_caput(&p, rsa_finis, &signum, &longitudo) < 0 || signum != 0x02)
        return -1;
    const uint8_t *mod_data = p;
    size_t mod_mag = longitudo;
    if (mod_mag > 0 && *mod_data == 0x00) { mod_data++; mod_mag--; }
    p += longitudo;

    /* exponens INTEGER */
    if (asn1_caput(&p, rsa_finis, &signum, &longitudo) < 0 || signum != 0x02)
        return -1;
    const uint8_t *exp_data = p;
    size_t exp_mag = longitudo;
    if (exp_mag > 0 && *exp_data == 0x00) { exp_data++; exp_mag--; }

    /* alloca et copia */
    clavis->modulus = malloc(mod_mag);
    clavis->exponens = malloc(exp_mag);
    if (!clavis->modulus || !clavis->exponens) {
        free(clavis->modulus);
        free(clavis->exponens);
        return -1;
    }
    memcpy(clavis->modulus, mod_data, mod_mag);
    clavis->modulus_mag = mod_mag;
    memcpy(clavis->exponens, exp_data, exp_mag);
    clavis->exponens_mag = exp_mag;
    return 0;
}

void asn1_libera(rsa_clavis_t *clavis)
{
    free(clavis->modulus);
    free(clavis->exponens);
    clavis->modulus = NULL;
    clavis->exponens = NULL;
}
