/*
 * summa.c — SHA-256 et HMAC-SHA-256
 *
 * Implementatio FIPS 180-4. Sine dependentiis externis.
 */

#include "arcana.h"
#include <string.h>

/* --- constantiae rotundae SHA-256 --- */

static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

/* --- operationes auxiliares --- */

#define DEXTRO(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define SIGMA0(x) (DEXTRO(x,2) ^ DEXTRO(x,13) ^ DEXTRO(x,22))
#define SIGMA1(x) (DEXTRO(x,6) ^ DEXTRO(x,11) ^ DEXTRO(x,25))
#define sigma0(x) (DEXTRO(x,7) ^ DEXTRO(x,18) ^ ((x) >> 3))
#define sigma1(x) (DEXTRO(x,17) ^ DEXTRO(x,19) ^ ((x) >> 10))
#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

static uint32_t lege32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8)  |  (uint32_t)p[3];
}

static void scribe32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

/* --- comprime unum truncum (64 octorum) --- */

static void comprime(uint32_t status[8], const uint8_t truncus[64])
{
    uint32_t W[64];
    for (int t = 0; t < 16; t++)
        W[t] = lege32(truncus + 4 * t);
    for (int t = 16; t < 64; t++)
        W[t] = sigma1(W[t-2]) + W[t-7] + sigma0(W[t-15]) + W[t-16];

    uint32_t a = status[0], b = status[1], c = status[2], d = status[3];
    uint32_t e = status[4], f = status[5], g = status[6], h = status[7];

    for (int t = 0; t < 64; t++) {
        uint32_t T1 = h + SIGMA1(e) + CH(e,f,g) + K[t] + W[t];
        uint32_t T2 = SIGMA0(a) + MAJ(a,b,c);
        h = g; g = f; f = e; e = d + T1;
        d = c; c = b; b = a; a = T1 + T2;
    }

    status[0] += a; status[1] += b; status[2] += c; status[3] += d;
    status[4] += e; status[5] += f; status[6] += g; status[7] += h;
}

/* --- interfacies publica SHA-256 --- */

void summa256_initia(summa256_ctx_t *ctx)
{
    ctx->status[0] = 0x6a09e667;
    ctx->status[1] = 0xbb67ae85;
    ctx->status[2] = 0x3c6ef372;
    ctx->status[3] = 0xa54ff53a;
    ctx->status[4] = 0x510e527f;
    ctx->status[5] = 0x9b05688c;
    ctx->status[6] = 0x1f83d9ab;
    ctx->status[7] = 0x5be0cd19;
    ctx->numerus_bitorum = 0;
    ctx->index_alvei = 0;
}

void summa256_adde(summa256_ctx_t *ctx, const uint8_t *data, size_t longitudo)
{
    for (size_t i = 0; i < longitudo; i++) {
        ctx->alveus[ctx->index_alvei++] = data[i];
        if (ctx->index_alvei == 64) {
            comprime(ctx->status, ctx->alveus);
            ctx->numerus_bitorum += 512;
            ctx->index_alvei = 0;
        }
    }
}

void summa256_fini(summa256_ctx_t *ctx, uint8_t digestum[32])
{
    ctx->numerus_bitorum += ctx->index_alvei * 8;

    /* complimentum: adde 1, deinde 0s, deinde longitudo */
    ctx->alveus[ctx->index_alvei++] = 0x80;
    if (ctx->index_alvei > 56) {
        while (ctx->index_alvei < 64)
            ctx->alveus[ctx->index_alvei++] = 0;
        comprime(ctx->status, ctx->alveus);
        ctx->index_alvei = 0;
    }
    while (ctx->index_alvei < 56)
        ctx->alveus[ctx->index_alvei++] = 0;

    /* longitudo in bitibus (big-endian 64 bit) */
    uint64_t nb = ctx->numerus_bitorum;
    for (int i = 7; i >= 0; i--)
        ctx->alveus[56 + (7 - i)] = (uint8_t)(nb >> (i * 8));

    comprime(ctx->status, ctx->alveus);

    for (int i = 0; i < 8; i++)
        scribe32(digestum + 4 * i, ctx->status[i]);
}

void summa256(const uint8_t *data, size_t longitudo, uint8_t digestum[32])
{
    summa256_ctx_t ctx;
    summa256_initia(&ctx);
    summa256_adde(&ctx, data, longitudo);
    summa256_fini(&ctx, digestum);
}

/* --- HMAC-SHA-256 (RFC 2104) --- */

void sigillum256(const uint8_t *clavis, size_t clavis_mag,
                 const uint8_t *nuntius, size_t nuntius_mag,
                 uint8_t mac[32])
{
    uint8_t clavis_completa[64];
    memset(clavis_completa, 0, 64);

    if (clavis_mag > 64) {
        summa256(clavis, clavis_mag, clavis_completa);
    } else {
        memcpy(clavis_completa, clavis, clavis_mag);
    }

    uint8_t alveus_internus[64], alveus_externus[64];
    for (int i = 0; i < 64; i++) {
        alveus_internus[i] = clavis_completa[i] ^ 0x36;
        alveus_externus[i] = clavis_completa[i] ^ 0x5c;
    }

    /* summa interna: H(clavis_i || nuntius) */
    summa256_ctx_t ctx;
    summa256_initia(&ctx);
    summa256_adde(&ctx, alveus_internus, 64);
    summa256_adde(&ctx, nuntius, nuntius_mag);

    uint8_t digestum_internum[32];
    summa256_fini(&ctx, digestum_internum);

    /* summa externa: H(clavis_e || digestum_internum) */
    summa256_initia(&ctx);
    summa256_adde(&ctx, alveus_externus, 64);
    summa256_adde(&ctx, digestum_internum, 32);
    summa256_fini(&ctx, mac);
}
