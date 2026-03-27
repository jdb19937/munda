/*
 * crispus/proba.c — probationes bibliothecae crispus
 *
 * Probat cryptographiam (SHA-256, AES-GCM, bignum, EC P-256)
 * et coniunctionem HTTPS ad servitores notos.
 */

#include "proba.h"
#include "crispus.h"
#include "internum.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int probationes_successae = 0;
static int probationes_defectae = 0;

#define PROBA(nomen, cond) do { \
    if (cond) { probationes_successae++; printf("  ✓ %s\n", nomen); } \
    else { probationes_defectae++; printf("  ✗ %s\n", nomen); } \
} while(0)

/* --- Probatio SHA-256 --- */

static void proba_summam(void)
{
    printf("SHA-256:\n");

    /* SHA-256("") = e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855 */
    {
        uint8_t digestum[32];
        summa256((const uint8_t *)"", 0, digestum);
        uint8_t expectatum[] = {
            0xe3,0xb0,0xc4,0x42,0x98,0xfc,0x1c,0x14,
            0x9a,0xfb,0xf4,0xc8,0x99,0x6f,0xb9,0x24,
            0x27,0xae,0x41,0xe4,0x64,0x9b,0x93,0x4c,
            0xa4,0x95,0x99,0x1b,0x78,0x52,0xb8,0x55
        };
        PROBA("SHA-256(\"\")", memcmp(digestum, expectatum, 32) == 0);
    }

    /* SHA-256("Gallia est omnis divisa in partes tres") */
    {
        uint8_t digestum[32];
        const char *gall = "Gallia est omnis divisa in partes tres";
        summa256((const uint8_t *)gall, strlen(gall), digestum);
        uint8_t expectatum[] = {
            0x04,0xaa,0x0e,0xfa,0x57,0x1a,0x7e,0xd6,
            0xca,0x18,0x10,0xf9,0x35,0xfb,0xda,0x33,
            0xa3,0xe2,0xc2,0x96,0xda,0x4c,0xca,0xe8,
            0x27,0x25,0x4c,0xbf,0x57,0x15,0x07,0x3a
        };
        PROBA("SHA-256(Gallia)", memcmp(digestum, expectatum, 32) == 0);
    }

    /* SHA-256 incrementalis */
    {
        uint8_t digestum1[32], digestum2[32];
        const char *nuntius = "Ars conjectandi est fundamentum calculi probabilitatum";
        summa256((const uint8_t *)nuntius, strlen(nuntius), digestum1);

        summa256_ctx_t ctx;
        summa256_initia(&ctx);
        summa256_adde(&ctx, (const uint8_t *)nuntius, 10);
        summa256_adde(&ctx, (const uint8_t *)nuntius + 10, strlen(nuntius) - 10);
        summa256_fini(&ctx, digestum2);
        PROBA("SHA-256 incrementalis", memcmp(digestum1, digestum2, 32) == 0);
    }
}

/* --- Probatio HMAC-SHA-256 --- */

static void proba_sigillum(void)
{
    printf("HMAC-SHA-256:\n");

    /* RFC 4231 test vector 1 */
    {
        uint8_t clavis[20];
        memset(clavis, 0x0b, 20);
        const uint8_t *data = (const uint8_t *)"Hi There";
        uint8_t mac[32];
        sigillum256(clavis, 20, data, 8, mac);
        uint8_t expectatum[] = {
            0xb0,0x34,0x4c,0x61,0xd8,0xdb,0x38,0x53,
            0x5c,0xa8,0xaf,0xce,0xaf,0x0b,0xf1,0x2b,
            0x88,0x1d,0xc2,0x00,0xc9,0x83,0x3d,0xa7,
            0x26,0xe9,0x37,0x6c,0x2e,0x32,0xcf,0xf7
        };
        PROBA("RFC 4231 vector 1", memcmp(mac, expectatum, 32) == 0);
    }
}

/* --- Probatio AES-128-GCM --- */

static void proba_arcam(void)
{
    printf("AES-128-GCM:\n");

    /* NIST test vector: AES-128-GCM */
    {
        /* clavis nulla, iv nulla, textus clarus vacuus */
        uint8_t clavis[16] = {0};
        uint8_t iv[12] = {0};
        uint8_t sigillum[16];

        arca128_gcm_occulta(clavis, iv, NULL, 0, NULL, 0, NULL, sigillum);

        uint8_t sig_expectatum[] = {
            0x58,0xe2,0xfc,0xce,0xfa,0x7e,0x30,0x61,
            0x36,0x7f,0x1d,0x57,0xa4,0xe7,0x45,0x5a
        };
        PROBA("GCM tag (vacuus)", memcmp(sigillum, sig_expectatum, 16) == 0);
    }

    /* occultatio et revelatio */
    {
        uint8_t clavis[16] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
        uint8_t iv[12] = {1,2,3,4,5,6,7,8,9,10,11,12};
        const char *textus = "Salve Munde!";
        size_t mag = strlen(textus);

        uint8_t occultus[64], revelatus[64], sigillum[16];
        arca128_gcm_occulta(clavis, iv,
                             (const uint8_t *)textus, mag,
                             NULL, 0, occultus, sigillum);

        /* textus occultus non debet esse idem ac clarus */
        PROBA("GCM occultat", memcmp(occultus, textus, mag) != 0);

        int rc = arca128_gcm_revela(clavis, iv, occultus, mag,
                                     NULL, 0, revelatus, sigillum);
        PROBA("GCM revelat (sigillum)", rc == 0);
        PROBA("GCM revelat (textus)", memcmp(revelatus, textus, mag) == 0);

        /* sigillum corruptum */
        uint8_t sig_malus[16];
        memcpy(sig_malus, sigillum, 16);
        sig_malus[0] ^= 0xff;
        rc = arca128_gcm_revela(clavis, iv, occultus, mag,
                                 NULL, 0, revelatus, sig_malus);
        PROBA("GCM reicit sigillum malum", rc != 0);
    }
}

/* --- Probatio Numeri Magni --- */

static void proba_numerum(void)
{
    printf("Numerus Magnus:\n");

    /* additio simplex */
    {
        nm_t a, b, r;
        nm_ex_nihilo(&a); a.v[0] = 0xFFFFFFFF;
        nm_ex_nihilo(&b); b.v[0] = 1;
        nm_adde(&r, &a, &b);
        PROBA("0xFFFFFFFF + 1 = 0x100000000", r.v[0] == 0 && r.v[1] == 1);
    }

    /* multiplicatio */
    {
        nm_t a, b, r;
        nm_ex_nihilo(&a); a.v[0] = 1000;
        nm_ex_nihilo(&b); b.v[0] = 1000;
        nm_multiplica(&r, &a, &b);
        PROBA("1000 * 1000 = 1000000", r.v[0] == 1000000);
    }

    /* divisio */
    {
        nm_t a, b, q, rem;
        nm_ex_nihilo(&a); a.v[0] = 17;
        nm_ex_nihilo(&b); b.v[0] = 5;
        nm_divide(&q, &rem, &a, &b);
        PROBA("17 / 5 = 3 rem 2", q.v[0] == 3 && rem.v[0] == 2);
    }

    /* divisio maior */
    {
        nm_t a, b, q, rem;
        nm_ex_nihilo(&a); a.v[0] = 0; a.v[1] = 1; a.n = 2; /* 2^32 */
        nm_ex_nihilo(&b); b.v[0] = 7;
        nm_divide(&q, &rem, &a, &b);
        /* 4294967296 / 7 = 613566756 rem 4 */
        PROBA("2^32 / 7 = 613566756 rem 4",
              q.v[0] == 613566756 && rem.v[0] == 4);
    }

    /* modpot: 2^10 mod 1000 = 24 */
    {
        nm_t basis, exp, mod, r;
        nm_ex_nihilo(&basis); basis.v[0] = 2;
        nm_ex_nihilo(&exp);   exp.v[0] = 10;
        nm_ex_nihilo(&mod);   mod.v[0] = 1000;
        nm_modpot(&r, &basis, &exp, &mod);
        PROBA("2^10 mod 1000 = 24", r.v[0] == 24);
    }

    /* conversio octorum */
    {
        uint8_t data[] = { 0x01, 0x00, 0x00, 0x00, 0x01 };
        nm_t a;
        nm_ex_octis(&a, data, 5);
        /* 0x0100000001 = 4294967297 = 2^32 + 1 */
        PROBA("nm_ex_octis(0x0100000001)", a.v[0] == 1 && a.v[1] == 1);

        uint8_t effectus[5];
        nm_ad_octos(&a, effectus, 5);
        PROBA("nm_ad_octos round-trip", memcmp(effectus, data, 5) == 0);
    }
}

/* --- Probatio EC P-256 --- */

static void proba_ec(void)
{
    printf("EC P-256:\n");

    /* G non est in infinito */
    PROBA("G non infinitum", EC_GENERATOR.infinitum == 0);

    /* 1 * G = G */
    {
        nm_t unum;
        nm_ex_nihilo(&unum);
        unum.v[0] = 1;
        ec_punctum_t r;
        ec_multiplica(&r, &unum, &EC_GENERATOR);
        PROBA("1*G = G", nm_compara(&r.x, &EC_GENERATOR.x) == 0 &&
                          nm_compara(&r.y, &EC_GENERATOR.y) == 0);
    }

    /* n * G = infinitum */
    {
        ec_punctum_t r;
        ec_multiplica(&r, &EC_ORDO, &EC_GENERATOR);
        PROBA("n*G = O (infinitum)", r.infinitum == 1);
    }

    /* 2*G: verificemus non est G neque infinitum */
    {
        nm_t duo;
        nm_ex_nihilo(&duo);
        duo.v[0] = 2;
        ec_punctum_t r;
        ec_multiplica(&r, &duo, &EC_GENERATOR);
        PROBA("2*G != G", nm_compara(&r.x, &EC_GENERATOR.x) != 0);
        PROBA("2*G != O", r.infinitum == 0);
    }
}

/* --- Probatio HTTPS --- */

static size_t proba_scribe_fn(void *data, size_t mag, size_t nmemb, void *usor)
{
    size_t realis = mag * nmemb;
    struct { char *data; size_t mag; } *acc = usor;
    char *novum = realloc(acc->data, acc->mag + realis + 1);
    if (!novum) return 0;
    acc->data = novum;
    memcpy(acc->data + acc->mag, data, realis);
    acc->mag += realis;
    acc->data[acc->mag] = '\0';
    return realis;
}

static void proba_https(void)
{
    printf("HTTPS:\n");

    crispus_orbis_initia(CRISPUS_GLOBAL_DEFAULT);

    /* GET https://www.google.com/ */
    {
        CRISPUS *c = crispus_facilis_initia();
        struct { char *data; size_t mag; } resp = { NULL, 0 };

        crispus_facilis_pone(c, CRISPUSOPT_URL, "https://www.google.com/");
        crispus_facilis_pone(c, CRISPUSOPT_FUNCTIO_SCRIBENDI, proba_scribe_fn);
        crispus_facilis_pone(c, CRISPUSOPT_DATA_SCRIBENDI, &resp);
        crispus_facilis_pone(c, CRISPUSOPT_TEMPUS, 15L);

        CRISPUScode rc = crispus_facilis_age(c);

        long codex = 0;
        crispus_facilis_info(c, CRISPUSINFO_CODEX_RESPONSI, &codex);

        printf("    rc=%d codex=%ld resp_mag=%zu\n", rc, codex, resp.mag);

        PROBA("google.com coniunctio", rc == CRISPUSE_OK);
        PROBA("google.com codex 200", codex == 200 || codex == 301 || codex == 302);
        PROBA("google.com habet corpus", resp.mag > 0);
        if (resp.data && resp.mag > 0)
            PROBA("google.com HTML", strstr(resp.data, "<") != NULL);

        free(resp.data);
        crispus_facilis_fini(c);
    }

    /* GET https://empslocal.ex.ac.uk/people/staff/mrwatkin/isoc/ (RSA-4096) */
    {
        CRISPUS *c = crispus_facilis_initia();
        struct { char *data; size_t mag; } resp = { NULL, 0 };

        crispus_facilis_pone(c, CRISPUSOPT_URL,
            "https://empslocal.ex.ac.uk/people/staff/mrwatkin/isoc/");
        crispus_facilis_pone(c, CRISPUSOPT_FUNCTIO_SCRIBENDI, proba_scribe_fn);
        crispus_facilis_pone(c, CRISPUSOPT_DATA_SCRIBENDI, &resp);
        crispus_facilis_pone(c, CRISPUSOPT_TEMPUS, 15L);

        CRISPUScode rc = crispus_facilis_age(c);

        long codex = 0;
        crispus_facilis_info(c, CRISPUSINFO_CODEX_RESPONSI, &codex);

        printf("    rc=%d codex=%ld resp_mag=%zu\n", rc, codex, resp.mag);

        PROBA("exeter.ac.uk coniunctio", rc == CRISPUSE_OK);
        PROBA("exeter.ac.uk codex 200", codex == 200);
        if (resp.data)
            PROBA("exeter.ac.uk HTML", strstr(resp.data, "<") != NULL);

        free(resp.data);
        crispus_facilis_fini(c);
    }

    /* GET https://plato.stanford.edu/entries/logic-modal/#TwoD */
    {
        CRISPUS *c = crispus_facilis_initia();
        struct { char *data; size_t mag; } resp = { NULL, 0 };

        crispus_facilis_pone(c, CRISPUSOPT_URL,
            "https://plato.stanford.edu/entries/logic-modal/#TwoD");
        crispus_facilis_pone(c, CRISPUSOPT_FUNCTIO_SCRIBENDI, proba_scribe_fn);
        crispus_facilis_pone(c, CRISPUSOPT_DATA_SCRIBENDI, &resp);
        crispus_facilis_pone(c, CRISPUSOPT_TEMPUS, 15L);

        CRISPUScode rc = crispus_facilis_age(c);

        long codex = 0;
        crispus_facilis_info(c, CRISPUSINFO_CODEX_RESPONSI, &codex);

        printf("    rc=%d codex=%ld resp_mag=%zu\n", rc, codex, resp.mag);

        PROBA("stanford.edu coniunctio", rc == CRISPUSE_OK);
        PROBA("stanford.edu codex 200", codex == 200);
        if (resp.data)
            PROBA("stanford.edu HTML", strstr(resp.data, "modal") != NULL);

        free(resp.data);
        crispus_facilis_fini(c);
    }

    /* GET https://www.fordcountychronicle.com/ */
    {
        CRISPUS *c = crispus_facilis_initia();
        struct { char *data; size_t mag; } resp = { NULL, 0 };

        crispus_facilis_pone(c, CRISPUSOPT_URL,
            "https://www.fordcountychronicle.com/articles/featured/naked-gunman-70-still-not-located/");
        crispus_facilis_pone(c, CRISPUSOPT_FUNCTIO_SCRIBENDI, proba_scribe_fn);
        crispus_facilis_pone(c, CRISPUSOPT_DATA_SCRIBENDI, &resp);
        crispus_facilis_pone(c, CRISPUSOPT_TEMPUS, 15L);

        CRISPUScode rc = crispus_facilis_age(c);

        long codex = 0;
        crispus_facilis_info(c, CRISPUSINFO_CODEX_RESPONSI, &codex);

        printf("    rc=%d codex=%ld resp_mag=%zu\n", rc, codex, resp.mag);

        PROBA("fordcountychronicle coniunctio", rc == CRISPUSE_OK);
        PROBA("fordcountychronicle codex", codex >= 200 && codex < 400);
        if (resp.data)
            PROBA("fordcountychronicle HTML", strstr(resp.data, "<") != NULL);

        free(resp.data);
        crispus_facilis_fini(c);
    }

    crispus_orbis_fini();
}

/* --- interfacies publica --- */

int crispus_proba(void)
{
    probationes_successae = 0;
    probationes_defectae = 0;

    printf("=== PROBATIONES CRISPUS ===\n\n");

    proba_summam();
    printf("\n");
    proba_sigillum();
    printf("\n");
    proba_arcam();
    printf("\n");
    proba_numerum();
    printf("\n");
    proba_ec();
    printf("\n");
    proba_https();

    printf("\n=== EFFECTUS: %d successae, %d defectae ===\n",
           probationes_successae, probationes_defectae);
    return probationes_defectae;
}
