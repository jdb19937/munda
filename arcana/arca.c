/*
 * arca.c — AES-128 et modus GCM
 *
 * Implementatio FIPS 197 (AES) et NIST SP 800-38D (GCM).
 * Sine dependentiis externis.
 */

#include "arcana.h"
#include <string.h>

/* --- Tabula S (SubBytes) --- */

static const uint8_t TABULA_S[256] = {
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
};

/* --- Constantiae rotundae (Rcon) --- */

static const uint8_t RCON[10] = {
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36
};

/* --- Expansio clavis --- */

static uint32_t verbum_sub(uint32_t w)
{
    return ((uint32_t)TABULA_S[(w >> 24) & 0xff] << 24) |
           ((uint32_t)TABULA_S[(w >> 16) & 0xff] << 16) |
           ((uint32_t)TABULA_S[(w >>  8) & 0xff] <<  8) |
           ((uint32_t)TABULA_S[ w        & 0xff]);
}

static uint32_t verbum_rota(uint32_t w)
{
    return (w << 8) | (w >> 24);
}

void arca128_expande(arca128_ctx_t *ctx, const uint8_t clavis[16])
{
    for (int i = 0; i < 4; i++)
        ctx->claves_expansae[i] = ((uint32_t)clavis[4*i] << 24) |
                                   ((uint32_t)clavis[4*i+1] << 16) |
                                   ((uint32_t)clavis[4*i+2] << 8) |
                                    (uint32_t)clavis[4*i+3];

    for (int i = 4; i < 44; i++) {
        uint32_t temp = ctx->claves_expansae[i - 1];
        if (i % 4 == 0)
            temp = verbum_sub(verbum_rota(temp)) ^ ((uint32_t)RCON[i/4 - 1] << 24);
        ctx->claves_expansae[i] = ctx->claves_expansae[i - 4] ^ temp;
    }
}

/* --- Occultatio unius trunci (128 bits) --- */

static void adde_clavem(uint8_t status[16], const uint32_t *clavis)
{
    for (int i = 0; i < 4; i++) {
        status[4*i]   ^= (uint8_t)(clavis[i] >> 24);
        status[4*i+1] ^= (uint8_t)(clavis[i] >> 16);
        status[4*i+2] ^= (uint8_t)(clavis[i] >> 8);
        status[4*i+3] ^= (uint8_t)(clavis[i]);
    }
}

static void sub_octos(uint8_t s[16])
{
    for (int i = 0; i < 16; i++)
        s[i] = TABULA_S[s[i]];
}

static void move_ordines(uint8_t s[16])
{
    uint8_t t;
    /* ordo 1: move sinistram 1 */
    t = s[1]; s[1] = s[5]; s[5] = s[9]; s[9] = s[13]; s[13] = t;
    /* ordo 2: move sinistram 2 */
    t = s[2]; s[2] = s[10]; s[10] = t;
    t = s[6]; s[6] = s[14]; s[14] = t;
    /* ordo 3: move sinistram 3 */
    t = s[3]; s[3] = s[15]; s[15] = s[11]; s[11] = s[7]; s[7] = t;
}

static uint8_t xtime(uint8_t a)
{
    return (uint8_t)((a << 1) ^ (((a >> 7) & 1) * 0x1b));
}

static void misce_columnas(uint8_t s[16])
{
    for (int i = 0; i < 4; i++) {
        uint8_t a = s[4*i], b = s[4*i+1], c = s[4*i+2], d = s[4*i+3];
        uint8_t e = a ^ b ^ c ^ d;
        s[4*i]   ^= e ^ xtime(a ^ b);
        s[4*i+1] ^= e ^ xtime(b ^ c);
        s[4*i+2] ^= e ^ xtime(c ^ d);
        s[4*i+3] ^= e ^ xtime(d ^ a);
    }
}

void arca128_occulta_truncum(const arca128_ctx_t *ctx,
                              const uint8_t in[16], uint8_t out[16])
{
    uint8_t status[16];
    memcpy(status, in, 16);

    adde_clavem(status, ctx->claves_expansae);

    for (int rotunda = 1; rotunda < 10; rotunda++) {
        sub_octos(status);
        move_ordines(status);
        misce_columnas(status);
        adde_clavem(status, ctx->claves_expansae + 4 * rotunda);
    }

    sub_octos(status);
    move_ordines(status);
    adde_clavem(status, ctx->claves_expansae + 40);

    memcpy(out, status, 16);
}

/* --- GCM --- */

/* multiplicatio in GF(2^128) */
static void gf128_multiplica(uint8_t r[16], const uint8_t x[16],
                              const uint8_t y[16])
{
    uint8_t v[16], z[16];
    memcpy(v, y, 16);
    memset(z, 0, 16);

    for (int i = 0; i < 128; i++) {
        if (x[i / 8] & (1 << (7 - i % 8))) {
            for (int j = 0; j < 16; j++)
                z[j] ^= v[j];
        }
        /* v = v * x mod P(x) */
        int portatio = v[15] & 1;
        for (int j = 15; j > 0; j--)
            v[j] = (v[j] >> 1) | ((v[j-1] & 1) << 7);
        v[0] >>= 1;
        if (portatio)
            v[0] ^= 0xe1;
    }
    memcpy(r, z, 16);
}

/* incrementum numeratoris (ultimi 4 octorum, big-endian) */
static void incrementa(uint8_t truncus[16])
{
    for (int i = 15; i >= 12; i--) {
        if (++truncus[i] != 0) break;
    }
}

/* GHASH: H = AES(K, 0^128), processus AAD et textus occultus */
static void ghash(const uint8_t H[16],
                  const uint8_t *aad, size_t aad_mag,
                  const uint8_t *occultus, size_t occ_mag,
                  uint8_t effectus[16])
{
    uint8_t X[16];
    memset(X, 0, 16);
    uint8_t truncus[16];

    /* processus AAD */
    size_t i;
    for (i = 0; i + 16 <= aad_mag; i += 16) {
        for (int j = 0; j < 16; j++)
            X[j] ^= aad[i + j];
        gf128_multiplica(truncus, X, H);
        memcpy(X, truncus, 16);
    }
    if (i < aad_mag) {
        memset(truncus, 0, 16);
        memcpy(truncus, aad + i, aad_mag - i);
        for (int j = 0; j < 16; j++)
            X[j] ^= truncus[j];
        gf128_multiplica(truncus, X, H);
        memcpy(X, truncus, 16);
    }

    /* processus textus occultus */
    for (i = 0; i + 16 <= occ_mag; i += 16) {
        for (int j = 0; j < 16; j++)
            X[j] ^= occultus[i + j];
        gf128_multiplica(truncus, X, H);
        memcpy(X, truncus, 16);
    }
    if (i < occ_mag) {
        memset(truncus, 0, 16);
        memcpy(truncus, occultus + i, occ_mag - i);
        for (int j = 0; j < 16; j++)
            X[j] ^= truncus[j];
        gf128_multiplica(truncus, X, H);
        memcpy(X, truncus, 16);
    }

    /* longitudines (in bitibus, big-endian 64 bit quisque) */
    uint8_t longitudines[16];
    memset(longitudines, 0, 16);
    uint64_t aad_bits = (uint64_t)aad_mag * 8;
    uint64_t occ_bits = (uint64_t)occ_mag * 8;
    for (int j = 0; j < 8; j++) {
        longitudines[j]     = (uint8_t)(aad_bits >> (56 - j * 8));
        longitudines[8 + j] = (uint8_t)(occ_bits >> (56 - j * 8));
    }
    for (int j = 0; j < 16; j++)
        X[j] ^= longitudines[j];
    gf128_multiplica(truncus, X, H);
    memcpy(effectus, truncus, 16);
}

/* --- AES-128-GCM occultatio --- */

int arca128_gcm_occulta(const uint8_t clavis[16], const uint8_t iv[12],
                         const uint8_t *clarus, size_t clarus_mag,
                         const uint8_t *aad, size_t aad_mag,
                         uint8_t *occultus, uint8_t sigillum[16])
{
    arca128_ctx_t ctx;
    arca128_expande(&ctx, clavis);

    /* H = AES(K, 0^128) */
    uint8_t H[16], nul[16];
    memset(nul, 0, 16);
    arca128_occulta_truncum(&ctx, nul, H);

    /* J0 = IV || 00000001 */
    uint8_t J0[16];
    memcpy(J0, iv, 12);
    J0[12] = 0; J0[13] = 0; J0[14] = 0; J0[15] = 1;

    /* occulta cum CTR (initium a J0 + 1) */
    uint8_t numerator[16];
    memcpy(numerator, J0, 16);

    for (size_t i = 0; i < clarus_mag; i += 16) {
        incrementa(numerator);
        uint8_t fluxus[16];
        arca128_occulta_truncum(&ctx, numerator, fluxus);
        size_t n = clarus_mag - i;
        if (n > 16) n = 16;
        for (size_t j = 0; j < n; j++)
            occultus[i + j] = clarus[i + j] ^ fluxus[j];
    }

    /* sigillum = GHASH(H, AAD, C) XOR AES(K, J0) */
    uint8_t S[16];
    ghash(H, aad, aad_mag, occultus, clarus_mag, S);
    uint8_t E_J0[16];
    arca128_occulta_truncum(&ctx, J0, E_J0);
    for (int j = 0; j < 16; j++)
        sigillum[j] = S[j] ^ E_J0[j];

    return 0;
}

/* --- AES-128-GCM revelatio --- */

int arca128_gcm_revela(const uint8_t clavis[16], const uint8_t iv[12],
                        const uint8_t *occultus, size_t occultus_mag,
                        const uint8_t *aad, size_t aad_mag,
                        uint8_t *clarus, const uint8_t sigillum[16])
{
    arca128_ctx_t ctx;
    arca128_expande(&ctx, clavis);

    uint8_t H[16], nul[16];
    memset(nul, 0, 16);
    arca128_occulta_truncum(&ctx, nul, H);

    uint8_t J0[16];
    memcpy(J0, iv, 12);
    J0[12] = 0; J0[13] = 0; J0[14] = 0; J0[15] = 1;

    /* verifica sigillum ante revelationem */
    uint8_t S[16];
    ghash(H, aad, aad_mag, occultus, occultus_mag, S);
    uint8_t E_J0[16];
    arca128_occulta_truncum(&ctx, J0, E_J0);
    uint8_t sigillum_computatum[16];
    for (int j = 0; j < 16; j++)
        sigillum_computatum[j] = S[j] ^ E_J0[j];

    /* comparatio temporis constantis */
    int diff = 0;
    for (int j = 0; j < 16; j++)
        diff |= sigillum_computatum[j] ^ sigillum[j];
    if (diff) return -1;

    /* revela cum CTR */
    uint8_t numerator[16];
    memcpy(numerator, J0, 16);

    for (size_t i = 0; i < occultus_mag; i += 16) {
        incrementa(numerator);
        uint8_t fluxus[16];
        arca128_occulta_truncum(&ctx, numerator, fluxus);
        size_t n = occultus_mag - i;
        if (n > 16) n = 16;
        for (size_t j = 0; j < n; j++)
            clarus[i + j] = occultus[i + j] ^ fluxus[j];
    }

    return 0;
}
