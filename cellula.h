/*
 * cellula.h — classis fundamentalis cellulae
 *
 * cellula_t continet solum campos radicis: genus, motum, pondus.
 * cella_t (in cella.h) extendit cum unione phylorum.
 */

#ifndef CELLULA_H
#define CELLULA_H

/* phyla cellularum */
typedef enum {
    FIXUM = 0,          /* terra: vacuum, saxum, murus */
    CIBUS,              /* olera edibilia: rapum, fungus */
    ANIMA,              /* viventia: feles, ursus, dalekus */
    DEI                 /* dei: zodus, oculus */
} phylum_t;

/* genera cellularum */
typedef enum {
    VACUUM = 0,         /* spatium vacuum */
    SAXUM,              /* lapis mobilis */
    FELES,              /* feles errans */
    DALEKUS,            /* bot quod oraculum groupnar rogat */
    URSUS,              /* ursus venator */
    MURUS,              /* murus immobilis */
    RAPUM,              /* rapum — satietas +1 */
    FUNGUS,             /* fungus — satietas +2 */
    ZODUS,              /* zodus — lusor, PELLE solum */
    OCULUS,             /* oculus — deus omnividens */
    CORVUS,             /* corvus — avis callida et rapax */
    GENERA_NUMERUS
} genus_t;

/* directiones */
typedef enum {
    DIR_NIHIL = 0,
    SEPTENTRIO,         /* sursum */
    MERIDIES,           /* deorsum */
    OCCIDENS,           /* sinistrorsum */
    ORIENS              /* dextrorsum */
} directio_t;

/* magnitudines sermonum et mentis */
#define SERMO_MAX  64           /* unum dictum */
#define AUDITA_MAX 256          /* receptaculum auditorum */
#define MENS_MAX   64           /* cogitationes currentis */

/* modi actionum */
typedef enum {
    QUIESCE = 0,        /* nihil agere */
    MOVE,               /* move ad vacuum tantum */
    PELLE,              /* pelle obiectum ante te */
    CAPE,               /* cape (ede) obiectum, move in locum eius */
    TRAHE,              /* move, trahe obiectum a tergo */
    LOQUERE,            /* loquere ad vicinum in directione — non moveris */
    CLAMA,              /* clama ad omnes in vicinitate 3×3 — non moveris */
    OPPUGNA             /* oppugna animum vicinum — vires > vitalitas requirit */
} modus_t;

/* actio composita: modus + directio + sermo + mens */
typedef struct {
    modus_t modus;
    directio_t directio;
    char sermo[SERMO_MAX];
    char mens[MENS_MAX];       /* nova mens — si [0]!=0, scribitur in cellam */
} actio_t;

#define ACTIO_NIHIL ((actio_t){ QUIESCE, DIR_NIHIL, {0}, {0} })

/* classis fundamentalis cellulae — solum campi radicis */
typedef struct cellula {
    genus_t genus;
    int motum;          /* iam acta hoc gradu */
    int pondus;         /* pondus cellulae */
} cellula_t;

#endif /* CELLULA_H */
