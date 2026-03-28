/*
 * arcana.h — primitiva cryptographica communia
 *
 * SHA-256, HMAC-SHA-256, AES-128-GCM, numerus magnus,
 * curva elliptica P-256, RSA, ASN.1/X.509, alea.
 *
 * Sine dependentiis externis.
 */

#ifndef ARCANA_H
#define ARCANA_H

#include <stddef.h>
#include <stdint.h>

/* --- SHA-256 --- */

typedef struct {
    uint32_t status[8];
    uint64_t numerus_bitorum;
    uint8_t  alveus[64];
    size_t   index_alvei;
} summa256_ctx_t;

void summa256_initia(summa256_ctx_t *ctx);
void summa256_adde(summa256_ctx_t *ctx, const uint8_t *data, size_t longitudo);
void summa256_fini(summa256_ctx_t *ctx, uint8_t digestum[32]);
void summa256(const uint8_t *data, size_t longitudo, uint8_t digestum[32]);

/* HMAC-SHA-256 */
void sigillum256(const uint8_t *clavis, size_t clavis_mag,
                 const uint8_t *nuntius, size_t nuntius_mag,
                 uint8_t mac[32]);

/* --- AES-128 --- */

typedef struct {
    uint32_t claves_expansae[44];
} arca128_ctx_t;

void arca128_expande(arca128_ctx_t *ctx, const uint8_t clavis[16]);
void arca128_occulta_truncum(const arca128_ctx_t *ctx,
                              const uint8_t in[16], uint8_t out[16]);

/* AES-128-GCM */
int arca128_gcm_occulta(const uint8_t clavis[16], const uint8_t iv[12],
                         const uint8_t *clarus, size_t clarus_mag,
                         const uint8_t *aad, size_t aad_mag,
                         uint8_t *occultus, uint8_t sigillum[16]);

int arca128_gcm_revela(const uint8_t clavis[16], const uint8_t iv[12],
                        const uint8_t *occultus, size_t occultus_mag,
                        const uint8_t *aad, size_t aad_mag,
                        uint8_t *clarus, const uint8_t sigillum[16]);

/* --- numerus magnus --- */

#define NM_VERBA 260   /* 260 * 32 = 8320 bits (sufficit productis RSA-4096) */

typedef struct {
    uint32_t v[NM_VERBA];
    int n;              /* verba activa */
} nm_t;

void nm_ex_nihilo(nm_t *a);
void nm_ex_octis(nm_t *a, const uint8_t *data, size_t mag);
void nm_ad_octos(const nm_t *a, uint8_t *data, size_t mag);
int  nm_compara(const nm_t *a, const nm_t *b);
void nm_adde(nm_t *r, const nm_t *a, const nm_t *b);
void nm_subtrahe(nm_t *r, const nm_t *a, const nm_t *b);
void nm_multiplica(nm_t *r, const nm_t *a, const nm_t *b);
void nm_divide(nm_t *q, nm_t *rem, const nm_t *a, const nm_t *b);
void nm_modulo(nm_t *r, const nm_t *a, const nm_t *m);
void nm_modmul(nm_t *r, const nm_t *a, const nm_t *b, const nm_t *m);
void nm_modpot(nm_t *r, const nm_t *basis, const nm_t *exponens,
               const nm_t *modulus);

/* --- curva elliptica P-256 --- */

typedef struct {
    nm_t x;
    nm_t y;
    int  infinitum;     /* punctum in infinito (elementum neutrum) */
} ec_punctum_t;

void ec_multiplica(ec_punctum_t *r, const nm_t *k, const ec_punctum_t *p);
void ec_adde(ec_punctum_t *r, const ec_punctum_t *a, const ec_punctum_t *b);

extern const ec_punctum_t EC_GENERATOR;
extern const nm_t EC_PRIMUS;
extern const nm_t EC_ORDO;

/* --- RSA --- */

typedef struct {
    uint8_t *modulus;
    size_t   modulus_mag;
    uint8_t *exponens;
    size_t   exponens_mag;
} rsa_clavis_t;

int rsa_verifica(const rsa_clavis_t *clavis,
                 const uint8_t *signatura, size_t sig_mag,
                 const uint8_t digestum[32]);

int rsa_occulta_pkcs1(const rsa_clavis_t *clavis,
                      const uint8_t *nuntius, size_t nun_mag,
                      uint8_t *occultus, size_t *occ_mag);

/* --- ASN.1 / X.509 --- */

int asn1_extrahe_rsa(const uint8_t *cert, size_t mag, rsa_clavis_t *clavis);
void asn1_libera(rsa_clavis_t *clavis);

/* --- alea --- */

int alea_imple(uint8_t *alveus, size_t mag);

#endif /* ARCANA_H */
