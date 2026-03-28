/*
 * retis.h — protocollum TCP tabulae mundae
 *
 * Framing, sessio cryptographica, handshake ECDHE,
 * certificatum I/O.
 *
 * Nulla dependentia a cella.h vel tabula.h —
 * potest includi ab et servitore et clientibus.
 */

#ifndef RETIS_H
#define RETIS_H

#include <stdint.h>
#include <stddef.h>
#include "arcana.h"

#define RETIS_PORTUS       7777
#define RETIS_NUNTIUS_MAX  131072
#define RETIS_CLIENTI_MAX  16

/* --- sessio cryptographica --- */

typedef struct {
    uint8_t clavis_scr[16];   /* clavis mea scribendi */
    uint8_t clavis_leg[16];   /* clavis paris legendi */
    uint8_t iv_scr[4];
    uint8_t iv_leg[4];
    uint64_t seq_scr;
    uint64_t seq_leg;
    int activa;               /* 1 post handshake */
} sessio_t;

/* --- alveus lectionis (pro partialibus TCP) --- */

typedef struct {
    uint8_t data[RETIS_NUNTIUS_MAX + 4];
    size_t  pos;
} alveus_retis_t;

/* --- framing: 4 octeti longitudo (big-endian) + payload --- */

int retis_mitte_nudum(int fd, const void *data, size_t mag);
int retis_mitte(int fd, sessio_t *ses, const void *clarus, size_t mag);

int retis_lege_frame(alveus_retis_t *alv, uint8_t **payload, size_t *mag);
void retis_alveus_consume(alveus_retis_t *alv, size_t consumpta);

/* decrypta frame in loco; reddit 0 = success, -1 = auth erratum */
int retis_revela(sessio_t *ses, uint8_t *frame, size_t frame_mag,
                 uint8_t **clarus, size_t *clar_mag);

/* --- handshake --- */

void retis_genera_clavem(nm_t *privata, ec_punctum_t *publica);

void retis_punctum_ad_hex(const ec_punctum_t *p, char *hex);
int  retis_hex_ad_punctum(const char *hex, ec_punctum_t *p);

void retis_deriva_claves(const ec_punctum_t *eph_communis,
                         const ec_punctum_t *stat_communis,
                         const ec_punctum_t *E_c,
                         const ec_punctum_t *E_s,
                         sessio_t *ses_c, sessio_t *ses_s);

/* --- certificatum (ISON) --- */

int retis_lege_certificatum(const char *via, ec_punctum_t *publica);
int retis_scribe_certificatum(const char *via, const ec_punctum_t *publica);
int retis_lege_clavem_secretam(const char *via, nm_t *privata,
                               ec_punctum_t *publica);
int retis_scribe_clavem_secretam(const char *via, const nm_t *privata,
                                 const ec_punctum_t *publica);

#endif /* RETIS_H */
