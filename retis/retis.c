/*
 * retis.c — protocollum TCP tabulae mundae
 *
 * framing, sessio AES-128-GCM, handshake ECDHE,
 * certificatum I/O in JSON.
 */

#include "retis.h"
#include "json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* --- framing --- */

static int mitte_plene(int fd, const void *data, size_t mag)
{
    const uint8_t *p = data;
    size_t missum = 0;
    while (missum < mag) {
        ssize_t r = write(fd, p + missum, mag - missum);
        if (r <= 0) return -1;
        missum += (size_t)r;
    }
    return 0;
}

int retis_mitte_nudum(int fd, const void *data, size_t mag)
{
    if (mag > RETIS_NUNTIUS_MAX) return -1;
    uint8_t caput[4];
    caput[0] = (uint8_t)(mag >> 24);
    caput[1] = (uint8_t)(mag >> 16);
    caput[2] = (uint8_t)(mag >> 8);
    caput[3] = (uint8_t)mag;
    if (mitte_plene(fd, caput, 4) < 0) return -1;
    if (mitte_plene(fd, data, mag) < 0) return -1;
    return 0;
}

int retis_mitte(int fd, sessio_t *ses, const void *clarus, size_t mag)
{
    if (!ses->activa)
        return retis_mitte_nudum(fd, clarus, mag);

    /* nonce: iv_scr(4) || seq_scr(8) = 12 octeti */
    uint8_t nonce[12];
    memcpy(nonce, ses->iv_scr, 4);
    uint64_t seq = ses->seq_scr;
    for (int i = 7; i >= 0; i--)
        nonce[4 + i] = (uint8_t)(seq >> ((7 - i) * 8));

    /* frame occultus: seq_explicitus(8) + textus_occultus(mag) + sigillum(16) */
    size_t frame_mag = 8 + mag + 16;
    uint8_t *frame = malloc(frame_mag);
    if (!frame) return -1;

    /* seq explicitus */
    for (int i = 7; i >= 0; i--)
        frame[i] = (uint8_t)(seq >> ((7 - i) * 8));

    /* occulta */
    arca128_gcm_occulta(ses->clavis_scr, nonce,
                        clarus, mag, NULL, 0,
                        frame + 8, frame + 8 + mag);

    ses->seq_scr++;

    int res = retis_mitte_nudum(fd, frame, frame_mag);
    free(frame);
    return res;
}

/* --- lectio cum alveo --- */

int retis_lege_frame(alveus_retis_t *alv, uint8_t **payload, size_t *mag)
{
    if (alv->pos < 4) return 0;

    uint32_t longitudo = ((uint32_t)alv->data[0] << 24) |
                          ((uint32_t)alv->data[1] << 16) |
                          ((uint32_t)alv->data[2] << 8)  |
                           (uint32_t)alv->data[3];

    if (longitudo > RETIS_NUNTIUS_MAX) return -1;
    if (alv->pos < 4 + longitudo) return 0;

    *payload = alv->data + 4;
    *mag = longitudo;
    return 1;
}

void retis_alveus_consume(alveus_retis_t *alv, size_t consumpta)
{
    size_t totum = 4 + consumpta;
    if (totum >= alv->pos) {
        alv->pos = 0;
    } else {
        memmove(alv->data, alv->data + totum, alv->pos - totum);
        alv->pos -= totum;
    }
}

int retis_revela(sessio_t *ses, uint8_t *frame, size_t frame_mag,
                 uint8_t **clarus, size_t *clar_mag)
{
    if (frame_mag < 8 + 16) return -1;

    /* reconstruere nonce ex seq explicito */
    uint8_t nonce[12];
    memcpy(nonce, ses->iv_leg, 4);
    memcpy(nonce + 4, frame, 8);

    size_t textus_mag = frame_mag - 8 - 16;
    uint8_t *textus_occultus = frame + 8;
    uint8_t *sigillum = frame + 8 + textus_mag;

    /* revela in loco (super textum occultum) */
    if (arca128_gcm_revela(ses->clavis_leg, nonce,
                           textus_occultus, textus_mag,
                           NULL, 0,
                           textus_occultus, sigillum) < 0)
        return -1;

    ses->seq_leg++;
    *clarus = textus_occultus;
    *clar_mag = textus_mag;
    return 0;
}

/* --- hex codificatio punctorum EC --- */

static const char HEX_TABULAE[] = "0123456789abcdef";

static void octeti_ad_hex(const uint8_t *src, size_t mag, char *dest)
{
    for (size_t i = 0; i < mag; i++) {
        dest[2*i]     = HEX_TABULAE[(src[i] >> 4) & 0xf];
        dest[2*i + 1] = HEX_TABULAE[src[i] & 0xf];
    }
}

static int hex_ad_octetos(const char *hex, size_t hex_mag,
                          uint8_t *dest, size_t dest_mag)
{
    if (hex_mag != dest_mag * 2) return -1;
    for (size_t i = 0; i < dest_mag; i++) {
        unsigned hi, lo;
        char c;
        c = hex[2*i];
        if      (c >= '0' && c <= '9') hi = (unsigned)(c - '0');
        else if (c >= 'a' && c <= 'f') hi = (unsigned)(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') hi = (unsigned)(c - 'A' + 10);
        else return -1;
        c = hex[2*i + 1];
        if      (c >= '0' && c <= '9') lo = (unsigned)(c - '0');
        else if (c >= 'a' && c <= 'f') lo = (unsigned)(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') lo = (unsigned)(c - 'A' + 10);
        else return -1;
        dest[i] = (uint8_t)((hi << 4) | lo);
    }
    return 0;
}

/* punctum EC non compressum: 04 || x(32) || y(32) = 65 octeti = 130 hex */
void retis_punctum_ad_hex(const ec_punctum_t *p, char *hex)
{
    uint8_t raw[65];
    raw[0] = 0x04;
    nm_ad_octos(&p->x, raw + 1, 32);
    nm_ad_octos(&p->y, raw + 33, 32);
    octeti_ad_hex(raw, 65, hex);
    hex[130] = '\0';
}

int retis_hex_ad_punctum(const char *hex, ec_punctum_t *p)
{
    if (strlen(hex) != 130) return -1;
    uint8_t raw[65];
    if (hex_ad_octetos(hex, 130, raw, 65) < 0) return -1;
    if (raw[0] != 0x04) return -1;
    nm_ex_octis(&p->x, raw + 1, 32);
    nm_ex_octis(&p->y, raw + 33, 32);
    p->infinitum = 0;
    return 0;
}

/* --- generatio clavis ephemerae --- */

void retis_genera_clavem(nm_t *privata, ec_punctum_t *publica)
{
    uint8_t octeti[32];
    alea_imple(octeti, 32);
    nm_ex_octis(privata, octeti, 32);

    /* reduc modulo EC_ORDO */
    nm_modulo(privata, privata, &EC_ORDO);

    /* publica = privata * G */
    ec_multiplica(publica, privata, &EC_GENERATOR);
}

/* --- derivatio clavium sessionis --- */

void retis_deriva_claves(const ec_punctum_t *eph_communis,
                         const ec_punctum_t *stat_communis,
                         const ec_punctum_t *E_c,
                         const ec_punctum_t *E_s,
                         sessio_t *ses_c, sessio_t *ses_s)
{
    /* praedominus = SHA-256(eph.x || stat.x) */
    uint8_t combinatio[64];
    nm_ad_octos(&eph_communis->x, combinatio, 32);
    nm_ad_octos(&stat_communis->x, combinatio + 32, 32);
    uint8_t praedominus[32];
    summa256(combinatio, 64, praedominus);

    /* semen = "claves tabula munda" || E_c.x || E_s.x */
    uint8_t semen[20 + 32 + 32];
    memcpy(semen, "claves tabula munda", 20);
    nm_ad_octos(&E_c->x, semen + 20, 32);
    nm_ad_octos(&E_s->x, semen + 52, 32);

    /* materia clavium = HMAC(praedominus, semen) */
    uint8_t materia[32];
    sigillum256(praedominus, 32, semen, sizeof(semen), materia);

    /* clavis_scr_c = [0..15], clavis_scr_s = [16..31] */
    memcpy(ses_c->clavis_scr, materia, 16);
    memcpy(ses_c->clavis_leg, materia + 16, 16);
    memcpy(ses_s->clavis_scr, materia + 16, 16);
    memcpy(ses_s->clavis_leg, materia, 16);

    /* IV: HMAC(praedominus, "iv tabula munda" || E_c.x || E_s.x) */
    uint8_t semen_iv[16 + 32 + 32];
    memcpy(semen_iv, "iv tabula munda!", 16);
    nm_ad_octos(&E_c->x, semen_iv + 16, 32);
    nm_ad_octos(&E_s->x, semen_iv + 48, 32);

    uint8_t iv_materia[32];
    sigillum256(praedominus, 32, semen_iv, sizeof(semen_iv), iv_materia);

    memcpy(ses_c->iv_scr, iv_materia, 4);
    memcpy(ses_c->iv_leg, iv_materia + 4, 4);
    memcpy(ses_s->iv_scr, iv_materia + 4, 4);
    memcpy(ses_s->iv_leg, iv_materia, 4);

    ses_c->seq_scr = 0;
    ses_c->seq_leg = 0;
    ses_s->seq_scr = 0;
    ses_s->seq_leg = 0;
    ses_c->activa = 1;
    ses_s->activa = 1;
}

/* --- certificatum I/O --- */

int retis_lege_certificatum(const char *via, ec_punctum_t *publica)
{
    char *json = json_lege_fasciculum(via);
    if (!json) return -1;

    char *hex = json_da_chordam(json, "clavis");
    free(json);
    if (!hex) return -1;

    int res = retis_hex_ad_punctum(hex, publica);
    free(hex);
    return res;
}

int retis_scribe_certificatum(const char *via, const ec_punctum_t *publica)
{
    char hex[131];
    retis_punctum_ad_hex(publica, hex);

    char json[256];
    snprintf(json, sizeof(json),
             "{\"clavis\":\"%s\",\"algorithmus\":\"EC-P256\"}", hex);

    FILE *f = fopen(via, "w");
    if (!f) return -1;
    fputs(json, f);
    fputc('\n', f);
    fclose(f);
    return 0;
}

int retis_lege_clavem_secretam(const char *via, nm_t *privata,
                               ec_punctum_t *publica)
{
    char *json = json_lege_fasciculum(via);
    if (!json) return -1;

    char *sec_hex = json_da_chordam(json, "secreta");
    char *pub_hex = json_da_chordam(json, "clavis");
    free(json);
    if (!sec_hex || !pub_hex) {
        free(sec_hex);
        free(pub_hex);
        return -1;
    }

    /* decodifica clavem secretam (64 hex = 32 octeti) */
    uint8_t octeti[32];
    int res = -1;
    if (strlen(sec_hex) == 64 &&
        hex_ad_octetos(sec_hex, 64, octeti, 32) == 0) {
        nm_ex_octis(privata, octeti, 32);
        res = retis_hex_ad_punctum(pub_hex, publica);
    }
    free(sec_hex);
    free(pub_hex);
    return res;
}

int retis_scribe_clavem_secretam(const char *via, const nm_t *privata,
                                 const ec_punctum_t *publica)
{
    char pub_hex[131];
    retis_punctum_ad_hex(publica, pub_hex);

    uint8_t octeti[32];
    nm_ad_octos(privata, octeti, 32);
    char sec_hex[65];
    octeti_ad_hex(octeti, 32, sec_hex);
    sec_hex[64] = '\0';

    char json[512];
    snprintf(json, sizeof(json),
             "{\"secreta\":\"%s\",\"clavis\":\"%s\"}", sec_hex, pub_hex);

    FILE *f = fopen(via, "w");
    if (!f) return -1;
    fputs(json, f);
    fputc('\n', f);
    fclose(f);
    return 0;
}
