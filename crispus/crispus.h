/*
 * crispus.h — bibliotheca retis HTTPS sine dependentiis externis
 *
 * Interfacies propria crispus.
 * Omnia cryptographica (SHA-256, AES-128-GCM, ECDHE, RSA) interne
 * implementantur. Solum libc POSIX requiritur.
 */

#ifndef CRISPUS_H
#define CRISPUS_H

#include <stddef.h>

/* --- typi opaci --- */

typedef void CRISPUS;        /* manubrium facile */
typedef void CRISPUSM;       /* manubrium multiplex */

typedef int CRISPUScode;
typedef int CRISPUSMcode;

/* --- codices exitus --- */

#define CRISPUSE_OK            0
#define CRISPUSE_ERRATUM       1
#define CRISPUSE_CONIUNCTIO    7
#define CRISPUSE_MEMORIA      27
#define CRISPUSE_TEMPUS       28

#define CRISPUSM_OK            0
#define CRISPUSM_ERRATUM      -1

/* --- optiones (ad crispus_facilis_pone) --- */

#define CRISPUSOPT_URL                  1
#define CRISPUSOPT_CAMPI_POSTAE         2
#define CRISPUSOPT_CAPITA_HTTP          3
#define CRISPUSOPT_FUNCTIO_SCRIBENDI    4
#define CRISPUSOPT_DATA_SCRIBENDI       5
#define CRISPUSOPT_TEMPUS               6

/* --- informationes (ad crispus_facilis_info) --- */

#define CRISPUSINFO_CODEX_RESPONSI      1

/* --- vexilla initialia --- */

#define CRISPUS_GLOBAL_DEFAULT          0

/* --- index capitum (slist) --- */

struct crispus_slist {
    char *data;
    struct crispus_slist *proximus;
};

/* --- nuntius multi --- */

#define CRISPUSMSG_PERFECTUM  1

typedef struct {
    int msg;
    CRISPUS *easy_handle;
    union {
        CRISPUScode result;
    } data;
} CRISPUSMsg;

/* --- functiones publicae --- */

CRISPUScode crispus_orbis_initia(long vexilla);
void        crispus_orbis_fini(void);

CRISPUS    *crispus_facilis_initia(void);
void        crispus_facilis_fini(CRISPUS *manubrium);
CRISPUScode crispus_facilis_pone(CRISPUS *manubrium, int optio, ...);
CRISPUScode crispus_facilis_age(CRISPUS *manubrium);
CRISPUScode crispus_facilis_info(CRISPUS *manubrium, int info, ...);
const char *crispus_facilis_error(CRISPUScode codex);

CRISPUSM   *crispus_multi_initia(void);
void        crispus_multi_fini(CRISPUSM *m);
CRISPUSMcode crispus_multi_adde(CRISPUSM *m, CRISPUS *facilis);
CRISPUSMcode crispus_multi_remove(CRISPUSM *m, CRISPUS *facilis);
CRISPUSMcode crispus_multi_age(CRISPUSM *m, int *currentes);
CRISPUSMsg  *crispus_multi_lege(CRISPUSM *m, int *residua);

struct crispus_slist *crispus_slist_adde(struct crispus_slist *index,
                                          const char *chorda);
void crispus_slist_libera(struct crispus_slist *index);

#endif /* CRISPUS_H */
