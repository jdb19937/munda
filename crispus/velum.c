/*
 * crispus_velum.c — TLS 1.2 (RFC 5246)
 *
 * Solum cipher: TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256 (0xc02f)
 * Curva: secp256r1 (P-256)
 *
 * Sine dependentiis externis.
 */

#include "internum.h"
#include "crispus.h"
#include "utilia.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

/* --- constantiae --- */

#define VERSIO_TLS12  0x0303
#define VERSIO_TLS10  0x0301

#define TABELLA_MUTATIO   20    /* ChangeCipherSpec */
#define TABELLA_SALUTATIO 22    /* Handshake */
#define TABELLA_APPLICATIO 23   /* Application Data */

#define SAL_SALVE_CLIENTIS   1
#define SAL_SALVE_SERVITORIS 2
#define SAL_TESTIMONIUM     11
#define SAL_CLAVIS_SERVITORIS 12
#define SAL_SALVE_FACTUM    14
#define SAL_CLAVIS_CLIENTIS 16
#define SAL_FINITUM         20

/* cipher: TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256 */
#define CIPHER_ECDHE 0xc02f

/* --- structura veli --- */

struct velum {
    int fd;
    char hospes[256];

    /* aleae */
    uint8_t clientis_alea[32];
    uint8_t servitoris_alea[32];

    /* clavis RSA servitoris (ex testimonio) */
    rsa_clavis_t clavis_rsa;
    int habet_clavem;

    /* ECDHE */
    nm_t ec_privata;
    ec_punctum_t ec_publica_servitoris;

    /* claves derivatae */
    uint8_t clavis_scr_c[16];   /* clavis scribendi clientis */
    uint8_t clavis_scr_s[16];   /* clavis scribendi servitoris */
    uint8_t iv_c[4];            /* IV implicita clientis */
    uint8_t iv_s[4];            /* IV implicita servitoris */

    /* numeri ordinis pro nonce GCM */
    uint64_t seq_c;
    uint64_t seq_s;

    /* transcriptum salutationis (SHA-256 currentis) */
    summa256_ctx_t transcriptum;

    /* secretum dominale */
    uint8_t secretum_dom[48];

    /* versio record layer (0x0301 initio, 0x0303 post ServerHello) */
    uint16_t versio_tabellae;

    /* status: 1 = post mutationem cipher */
    int occultans;    /* scribimus cum crypto */
    int revelans;     /* legimus cum crypto */

    /* alveus salutationis (accumulat nuntios trans tabellas) */
    uint8_t alveus_sal[32768];
    size_t sal_pos;
    size_t sal_mag;

    /* alveus applicationis (data revelata) */
    uint8_t alveus_app[16384 + 256];
    size_t app_pos;
    size_t app_mag;
};


/* --- TLS record mittere --- */

static int mitte_tabellam(velum_t *v, uint8_t genus, const uint8_t *data,
                           size_t mag)
{
    uint8_t caput[5];
    caput[0] = genus;
    scr16(caput + 1, v->versio_tabellae);
    scr16(caput + 3, (uint16_t)mag);
    if (mitte_plene(v->fd, caput, 5) < 0) return -1;
    if (mag > 0 && mitte_plene(v->fd, data, mag) < 0) return -1;
    return 0;
}

/* mitte tabellam occultam (AES-128-GCM) */
static int mitte_tabellam_occultam(velum_t *v, uint8_t genus,
                                    const uint8_t *data, size_t mag)
{
    /* nonce = iv_c (4) || explicit_nonce (8) */
    uint8_t nonce[12];
    memcpy(nonce, v->iv_c, 4);
    for (int i = 7; i >= 0; i--)
        nonce[4 + (7 - i)] = (uint8_t)(v->seq_c >> (i * 8));

    /* AAD: seq (8) || genus (1) || versio (2) || longitudo (2) */
    uint8_t aad[13];
    for (int i = 7; i >= 0; i--)
        aad[7 - i] = (uint8_t)(v->seq_c >> (i * 8));
    aad[8] = genus;
    scr16(aad + 9, VERSIO_TLS12);
    scr16(aad + 11, (uint16_t)mag);

    /* occulta */
    uint8_t *occultus = malloc(8 + mag + 16);
    if (!occultus) return -1;

    /* explicit nonce (8 octorum) */
    memcpy(occultus, nonce + 4, 8);

    arca128_gcm_occulta(v->clavis_scr_c, nonce,
                         data, mag, aad, 13,
                         occultus + 8, occultus + 8 + mag);

    int rc = mitte_tabellam(v, genus, occultus, 8 + mag + 16);
    free(occultus);
    v->seq_c++;
    return rc;
}

/* --- TLS record legere --- */

static int lege_tabellam(velum_t *v, uint8_t *genus, uint8_t *data,
                          size_t *mag)
{
    uint8_t caput[5];
    if (lege_plene(v->fd, caput, 5) < 0) return -1;
    *genus = caput[0];
    *mag = leg16(caput + 3);
    if (*mag > 16384 + 256) return -1;
    if (lege_plene(v->fd, data, *mag) < 0) return -1;
    return 0;
}

/* lege et revela tabellam */
static int lege_tabellam_revelam(velum_t *v, uint8_t *genus,
                                  uint8_t *data, size_t *mag)
{
    uint8_t alveus[16384 + 256];
    size_t alveus_mag;
    if (lege_tabellam(v, genus, alveus, &alveus_mag) < 0) return -1;

    if (!v->revelans) {
        memcpy(data, alveus, alveus_mag);
        *mag = alveus_mag;
        return 0;
    }

    /* revelatio GCM: alveus = explicit_nonce(8) || ciphertext || tag(16) */
    if (alveus_mag < 24) return -1;

    uint8_t nonce[12];
    memcpy(nonce, v->iv_s, 4);
    memcpy(nonce + 4, alveus, 8);

    size_t tc_mag = alveus_mag - 8 - 16;

    /* AAD */
    uint8_t aad[13];
    for (int i = 7; i >= 0; i--)
        aad[7 - i] = (uint8_t)(v->seq_s >> (i * 8));
    aad[8] = *genus;
    scr16(aad + 9, VERSIO_TLS12);
    scr16(aad + 11, (uint16_t)tc_mag);

    if (arca128_gcm_revela(v->clavis_scr_s, nonce,
                            alveus + 8, tc_mag,
                            aad, 13, data,
                            alveus + 8 + tc_mag) < 0)
        return -1;

    *mag = tc_mag;
    v->seq_s++;
    return 0;
}

/* --- TLS PRF (SHA-256) --- */

static void prf(const uint8_t *secretum, size_t sec_mag,
                const char *titulus,
                const uint8_t *semen, size_t sem_mag,
                uint8_t *effectus, size_t eff_mag)
{
    size_t tit_mag = strlen(titulus);

    /* semen completum = titulus || semen */
    size_t sc_mag = tit_mag + sem_mag;
    uint8_t *sc = malloc(sc_mag);
    if (!sc) return;
    memcpy(sc, titulus, tit_mag);
    memcpy(sc + tit_mag, semen, sem_mag);

    /* A(0) = sc, A(i) = HMAC(secretum, A(i-1)) */
    uint8_t A[32];
    sigillum256(secretum, sec_mag, sc, sc_mag, A);

    size_t scriptum = 0;
    while (scriptum < eff_mag) {
        /* P(i) = HMAC(secretum, A(i) || sc) */
        uint8_t *concatenatio = malloc(32 + sc_mag);
        if (!concatenatio) break;
        memcpy(concatenatio, A, 32);
        memcpy(concatenatio + 32, sc, sc_mag);

        uint8_t P[32];
        sigillum256(secretum, sec_mag, concatenatio, 32 + sc_mag, P);
        free(concatenatio);

        size_t n = eff_mag - scriptum;
        if (n > 32) n = 32;
        memcpy(effectus + scriptum, P, n);
        scriptum += n;

        /* A(i+1) = HMAC(secretum, A(i)) */
        uint8_t A_novum[32];
        sigillum256(secretum, sec_mag, A, 32, A_novum);
        memcpy(A, A_novum, 32);
    }
    free(sc);
}

/* --- adde ad transcriptum salutationis --- */

static void transcribe(velum_t *v, const uint8_t *data, size_t mag)
{
    summa256_adde(&v->transcriptum, data, mag);
}

/* --- constructio ClientHello --- */

static int mitte_salve_clientis(velum_t *v)
{
    alea_imple(v->clientis_alea, 32);

    /* extensiones */
    size_t hospes_mag = strlen(v->hospes);

    /* SNI */
    uint8_t ext_sni[256];
    size_t sni_mag = 0;
    scr16(ext_sni, 0x0000);           /* genus: server_name */
    sni_mag = 2;
    size_t sni_data_mag = 2 + 1 + 2 + hospes_mag;
    scr16(ext_sni + 2, (uint16_t)sni_data_mag);
    sni_mag += 2;
    scr16(ext_sni + 4, (uint16_t)(sni_data_mag - 2));
    sni_mag += 2;
    ext_sni[6] = 0x00;                /* genus: host_name */
    sni_mag++;
    scr16(ext_sni + 7, (uint16_t)hospes_mag);
    sni_mag += 2;
    memcpy(ext_sni + 9, v->hospes, hospes_mag);
    sni_mag += hospes_mag;

    /* supported_groups: secp256r1 */
    uint8_t ext_groups[] = {
        0x00, 0x0a,              /* genus */
        0x00, 0x04,              /* longitudo */
        0x00, 0x02,              /* index longitudo */
        0x00, 0x17               /* secp256r1 */
    };

    /* ec_point_formats: uncompressed */
    uint8_t ext_ecf[] = {
        0x00, 0x0b,
        0x00, 0x02,
        0x01,
        0x00                     /* uncompressed */
    };

    /* signature_algorithms: rsa_pkcs1_sha256 solum */
    uint8_t ext_sig[] = {
        0x00, 0x0d,
        0x00, 0x04,
        0x00, 0x02,
        0x04, 0x01               /* rsa_pkcs1_sha256 */
    };

    /* renegotiation_info (vacua) */
    uint8_t ext_reneg[] = {
        0xff, 0x01,
        0x00, 0x01,
        0x00
    };

    size_t ext_totalis = sni_mag + sizeof(ext_groups) + sizeof(ext_ecf) +
                         sizeof(ext_sig) + sizeof(ext_reneg);

    /* corpus ClientHello */
    /* 2(versio) + 32(alea) + 1(sid_mag) + 2(cs_mag) + 4(cs) +
     * 1(comp_mag) + 1(comp) + 2(ext_mag) + extensiones */
    size_t corpus_mag = 2 + 32 + 1 + 2 + 4 + 1 + 1 + 2 + ext_totalis;

    /* nuntius salutationis = genus(1) + longitudo(3) + corpus */
    size_t nuntius_mag = 4 + corpus_mag;
    uint8_t *nuntius = malloc(nuntius_mag);
    if (!nuntius) return -1;

    uint8_t *p = nuntius;

    /* caput salutationis */
    *p++ = SAL_SALVE_CLIENTIS;
    scr24(p, (uint32_t)corpus_mag); p += 3;

    /* versio clientis */
    scr16(p, VERSIO_TLS12); p += 2;

    /* alea clientis */
    memcpy(p, v->clientis_alea, 32); p += 32;

    /* sessio ID (vacua) */
    *p++ = 0;

    /* cipher suites */
    scr16(p, 4); p += 2;          /* 2 cipher suites * 2 bytes */
    scr16(p, CIPHER_ECDHE); p += 2;
    scr16(p, 0x00ff); p += 2;      /* TLS_EMPTY_RENEGOTIATION_INFO_SCSV */

    /* compressio: nulla */
    *p++ = 1; *p++ = 0;

    /* extensiones */
    scr16(p, (uint16_t)ext_totalis); p += 2;
    memcpy(p, ext_sni, sni_mag); p += sni_mag;
    memcpy(p, ext_groups, sizeof(ext_groups)); p += sizeof(ext_groups);
    memcpy(p, ext_ecf, sizeof(ext_ecf)); p += sizeof(ext_ecf);
    memcpy(p, ext_sig, sizeof(ext_sig)); p += sizeof(ext_sig);
    memcpy(p, ext_reneg, sizeof(ext_reneg)); p += sizeof(ext_reneg);

    (void)p;

    /* adde ad transcriptum */
    transcribe(v, nuntius, nuntius_mag);

    /* involve in tabella TLS */
    int rc = mitte_tabellam(v, TABELLA_SALUTATIO, nuntius, nuntius_mag);
    free(nuntius);
    return rc;
}

/* --- legere nuntios salutationis a servitore --- */

/* lege unum nuntium salutationis (potest legere plures tabellas) */
static int sal_imple(velum_t *v, size_t opus)
{
    while (v->sal_mag - v->sal_pos < opus) {
        uint8_t gen_tab;
        uint8_t alveus[16384 + 256];
        size_t alveus_mag;
        if (lege_tabellam(v, &gen_tab, alveus, &alveus_mag) < 0)
            return -1;
        if (gen_tab != TABELLA_SALUTATIO)
            return -1;
        /* compacta si opus est */
        if (v->sal_pos > 0 && v->sal_mag + alveus_mag > sizeof(v->alveus_sal)) {
            memmove(v->alveus_sal, v->alveus_sal + v->sal_pos,
                    v->sal_mag - v->sal_pos);
            v->sal_mag -= v->sal_pos;
            v->sal_pos = 0;
        }
        if (v->sal_mag + alveus_mag > sizeof(v->alveus_sal)) return -1;
        memcpy(v->alveus_sal + v->sal_mag, alveus, alveus_mag);
        v->sal_mag += alveus_mag;
    }
    return 0;
}

static int lege_nuntium_sal(velum_t *v, uint8_t *genus, uint8_t *data,
                             size_t *mag)
{
    /* assure caput salutationis (4 octorum) */
    if (sal_imple(v, 4) < 0) return -1;

    uint8_t *p = v->alveus_sal + v->sal_pos;
    *genus = p[0];
    size_t lon = leg24(p + 1);

    /* assure corpus integrum */
    if (sal_imple(v, 4 + lon) < 0) return -1;

    p = v->alveus_sal + v->sal_pos;
    memcpy(data, p + 4, lon);
    *mag = lon;

    /* adde ad transcriptum */
    transcribe(v, p, 4 + lon);

    v->sal_pos += 4 + lon;
    if (v->sal_pos == v->sal_mag)
        v->sal_pos = v->sal_mag = 0;

    return 0;
}

/* --- processus salutationis --- */

velum_t *velum_crea(int fd, const char *hospes)
{
    velum_t *v = calloc(1, sizeof(velum_t));
    if (!v) return NULL;
    v->fd = fd;
    if (hospes) {
        size_t n = strlen(hospes);
        if (n >= sizeof(v->hospes)) n = sizeof(v->hospes) - 1;
        memcpy(v->hospes, hospes, n);
    }
    v->versio_tabellae = VERSIO_TLS10;
    summa256_initia(&v->transcriptum);
    return v;
}

int velum_saluta(velum_t *v)
{
    uint8_t data[32768];
    size_t mag;
    uint8_t genus;

    /* 1. mitte ClientHello */
    if (mitte_salve_clientis(v) < 0) return -1;

    /* 2. accipe ServerHello */
    if (lege_nuntium_sal(v, &genus, data, &mag) < 0 ||
        genus != SAL_SALVE_SERVITORIS)
        return -1;
    if (mag < 38) return -1;
    /* versio (2) + alea (32) + sessio_id_mag (1) ... */
    memcpy(v->servitoris_alea, data + 2, 32);
    size_t sid_mag = data[34];
    size_t pos = 35 + sid_mag;
    if (pos + 3 > mag) return -1;
    uint16_t cipher = leg16(data + pos);
    if (cipher != CIPHER_ECDHE) return -1;
    v->versio_tabellae = VERSIO_TLS12;

    /* 3. accipe Certificate */
    if (lege_nuntium_sal(v, &genus, data, &mag) < 0 ||
        genus != SAL_TESTIMONIUM)
        return -1;
    /* processus catena testimoniorum */
    if (mag < 3) return -1;
    size_t catena_mag = leg24(data);
    if (catena_mag + 3 > mag) return -1;
    /* primum testimonium */
    if (catena_mag < 3) return -1;
    size_t test_mag = leg24(data + 3);
    if (test_mag + 6 > mag) return -1;
    /* extrahe clavem RSA */
    if (asn1_extrahe_rsa(data + 6, test_mag, &v->clavis_rsa) < 0)
        return -1;
    v->habet_clavem = 1;

    /* 4. accipe ServerKeyExchange */
    if (lege_nuntium_sal(v, &genus, data, &mag) < 0 ||
        genus != SAL_CLAVIS_SERVITORIS)
        return -1;
    /* parsamus parametra EC */
    if (mag < 4) return -1;
    if (data[0] != 0x03) return -1;            /* named_curve */
    if (leg16(data + 1) != 0x0017) return -1;  /* secp256r1 */
    size_t pub_mag = data[3];
    if (pub_mag != 65 || mag < 4 + 65) return -1;
    if (data[4] != 0x04) return -1;            /* uncompressed */
    nm_ex_octis(&v->ec_publica_servitoris.x, data + 5, 32);
    nm_ex_octis(&v->ec_publica_servitoris.y, data + 37, 32);
    v->ec_publica_servitoris.infinitum = 0;

    /* verifica signaturam (optionalis pro celeritate — verificamus) */
    size_t params_mag = 4 + pub_mag;
    size_t sig_offset = params_mag;
    if (sig_offset + 4 > mag) return -1;
    /* algorithmus signaturae (2 octorum) */
    sig_offset += 2;
    size_t sig_mag = leg16(data + sig_offset);
    sig_offset += 2;
    if (sig_offset + sig_mag > mag) return -1;

    /* digestum: SHA256(clientis_alea || servitoris_alea || params) */
    {
        summa256_ctx_t ctx;
        summa256_initia(&ctx);
        summa256_adde(&ctx, v->clientis_alea, 32);
        summa256_adde(&ctx, v->servitoris_alea, 32);
        summa256_adde(&ctx, data, params_mag);
        uint8_t dig[32];
        summa256_fini(&ctx, dig);

        if (rsa_verifica(&v->clavis_rsa, data + sig_offset, sig_mag, dig) < 0)
            return -1;
    }

    /* 5. accipe ServerHelloDone */
    if (lege_nuntium_sal(v, &genus, data, &mag) < 0 ||
        genus != SAL_SALVE_FACTUM)
        return -1;

    /* 6. genera clavis ECDHE clientis */
    uint8_t privata_octis[32];
    alea_imple(privata_octis, 32);
    nm_ex_octis(&v->ec_privata, privata_octis, 32);

    /* clavis publica clientis = ec_privata * G */
    ec_punctum_t publica_clientis;
    ec_multiplica(&publica_clientis, &v->ec_privata, &EC_GENERATOR);

    /* 7. mitte ClientKeyExchange */
    {
        uint8_t cke[70];
        cke[0] = SAL_CLAVIS_CLIENTIS;
        scr24(cke + 1, 66);  /* longitudo: 1 + 65 */
        cke[4] = 65;          /* longitudo puncti */
        cke[5] = 0x04;        /* uncompressed */
        nm_ad_octos(&publica_clientis.x, cke + 6, 32);
        nm_ad_octos(&publica_clientis.y, cke + 38, 32);

        transcribe(v, cke, 70);
        if (mitte_tabellam(v, TABELLA_SALUTATIO, cke, 70) < 0)
            return -1;
    }
    /* 8. computa secretum commune */
    ec_punctum_t punctum_commune;
    ec_multiplica(&punctum_commune, &v->ec_privata, &v->ec_publica_servitoris);
    uint8_t praedominus[32];
    nm_ad_octos(&punctum_commune.x, praedominus, 32);

    /* 9. computa secretum dominale */
    uint8_t semen[64];
    memcpy(semen, v->clientis_alea, 32);
    memcpy(semen + 32, v->servitoris_alea, 32);
    prf(praedominus, 32, "master secret", semen, 64, v->secretum_dom, 48);

    /* 10. deriva claves */
    uint8_t semen_exp[64];
    memcpy(semen_exp, v->servitoris_alea, 32);
    memcpy(semen_exp + 32, v->clientis_alea, 32);

    uint8_t materia[40];  /* 16+16+4+4 = 40 */
    prf(v->secretum_dom, 48, "key expansion", semen_exp, 64, materia, 40);

    memcpy(v->clavis_scr_c, materia, 16);
    memcpy(v->clavis_scr_s, materia + 16, 16);
    memcpy(v->iv_c, materia + 32, 4);
    memcpy(v->iv_s, materia + 36, 4);

    /* 11. mitte ChangeCipherSpec */
    {
        uint8_t ccs = 1;
        if (mitte_tabellam(v, TABELLA_MUTATIO, &ccs, 1) < 0)
            return -1;
        v->occultans = 1;
    }

    /* 12. mitte Finished (occultum) */
    {
        /* verify_data = PRF(master_secret, "client finished", Hash(transcriptum))[0..11] */
        summa256_ctx_t copia = v->transcriptum;
        uint8_t digestum[32];
        summa256_fini(&copia, digestum);

        uint8_t verify_data[12];
        prf(v->secretum_dom, 48, "client finished", digestum, 32,
            verify_data, 12);

        uint8_t finitum[16];
        finitum[0] = SAL_FINITUM;
        scr24(finitum + 1, 12);
        memcpy(finitum + 4, verify_data, 12);

        /* adde ad transcriptum ANTE occultationem */
        transcribe(v, finitum, 16);

        /* mitte occultum */
        if (mitte_tabellam_occultam(v, TABELLA_SALUTATIO, finitum, 16) < 0)
            return -1;
    }

    /* 13. accipe ChangeCipherSpec servitoris */
    {
        uint8_t gen_tab;
        uint8_t alveus[16];
        size_t alveus_mag;
        if (lege_tabellam(v, &gen_tab, alveus, &alveus_mag) < 0) return -1;
        if (gen_tab != TABELLA_MUTATIO) return -1;
        v->revelans = 1;
    }

    /* 14. accipe Finished servitoris (occultum) */
    {
        uint8_t gen_tab;
        uint8_t alveus[256];
        size_t alveus_mag;
        if (lege_tabellam_revelam(v, &gen_tab, alveus, &alveus_mag) < 0)
            return -1;
        if (gen_tab != TABELLA_SALUTATIO) return -1;
        if (alveus_mag < 16) return -1;
        if (alveus[0] != SAL_FINITUM) return -1;
        /* verificamus verify_data servitoris */
        summa256_ctx_t copia = v->transcriptum;
        uint8_t digestum[32];
        summa256_fini(&copia, digestum);

        uint8_t verify_expectatum[12];
        prf(v->secretum_dom, 48, "server finished", digestum, 32,
            verify_expectatum, 12);

        if (memcmp(alveus + 4, verify_expectatum, 12) != 0)
            return -1;
    }

    v->app_pos = v->app_mag = 0;
    return 0;
}

/* --- data applicationis --- */

int velum_scribe(velum_t *v, const void *data, size_t mag)
{
    const uint8_t *p = data;
    while (mag > 0) {
        size_t n = mag > 16384 ? 16384 : mag;
        if (mitte_tabellam_occultam(v, TABELLA_APPLICATIO, p, n) < 0)
            return -1;
        p += n;
        mag -= n;
    }
    return 0;
}

int velum_lege(velum_t *v, void *alveus, size_t mag)
{
    uint8_t *dest = alveus;
    size_t lectum = 0;

    while (lectum < mag) {
        /* si habemus data in alveo applicationis, utere */
        if (v->app_pos < v->app_mag) {
            size_t disponibilia = v->app_mag - v->app_pos;
            size_t n = mag - lectum;
            if (n > disponibilia) n = disponibilia;
            memcpy(dest + lectum, v->alveus_app + v->app_pos, n);
            v->app_pos += n;
            lectum += n;
            continue;
        }

        /* lege novam tabellam */
        uint8_t genus;
        uint8_t data_tab[16384 + 256];
        size_t data_mag;
        if (lege_tabellam_revelam(v, &genus, data_tab, &data_mag) < 0)
            return (int)lectum > 0 ? (int)lectum : -1;

        if (genus != TABELLA_APPLICATIO)
            continue;   /* praetermitte non-applicationalia */

        memcpy(v->alveus_app, data_tab, data_mag);
        v->app_pos = 0;
        v->app_mag = data_mag;
    }
    return (int)lectum;
}

void velum_claude(velum_t *v)
{
    if (!v) return;
    /* mitte close_notify (optionalis) */
    if (v->occultans) {
        uint8_t alerta[2] = { 1, 0 };  /* warning, close_notify */
        mitte_tabellam_occultam(v, 21, alerta, 2);
    }
    if (v->habet_clavem)
        asn1_libera(&v->clavis_rsa);
    free(v);
}
