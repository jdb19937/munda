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
    OPPUGNA,            /* oppugna animum vicinum — vires > vitalitas requirit */
    TELEPORTA           /* teleporta ad coordinatas — dei potentia > potentia, animi vires > vitalitas */
} modus_t;

/* larvae capacitatum — quos modos genus agere potest */
#define CAP_MOVE      (1u << MOVE)
#define CAP_PELLE     (1u << PELLE)
#define CAP_CAPE      (1u << CAPE)
#define CAP_TRAHE     (1u << TRAHE)
#define CAP_LOQUERE   (1u << LOQUERE)
#define CAP_CLAMA     (1u << CLAMA)
#define CAP_OPPUGNA   (1u << OPPUGNA)
#define CAP_TELEPORTA (1u << TELEPORTA)

#define CAP_ANIMA (CAP_MOVE | CAP_PELLE | CAP_CAPE | CAP_TRAHE | \
                   CAP_LOQUERE | CAP_CLAMA | CAP_OPPUGNA)
#define CAP_DEI   (CAP_ANIMA | CAP_TELEPORTA)

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
