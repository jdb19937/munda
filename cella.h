/*
 * cella.h — cellula plena cum unione phylorum
 *
 * Hierarchia classium:
 *   cellula_t            radix — genus, motum, pondus
 *   cella_t              cellula + unio phylorum
 *     fixum_t              phylum FIXUM — vacuum, saxum, murus
 *       vacuum_t
 *       saxum_t
 *       murus_t
 *     cibus_t              phylum CIBUS — rapum, fungus
 *       rapum_t
 *       fungus_t
 *     animus_t             phylum ANIMA — feles, dalekus, ursus, corvus
 *       feles_t
 *       dalekus_t
 *       ursus_t
 *       corvus_t
 *     deus_t               phylum DEI — zodus, oculus
 *       zodus_t
 *       oculus_t
 *
 * Phyla: FIXUM (terra), CIBUS (olera), ANIMA (viventia), DEI (dei).
 */

#ifndef CELLA_H
#define CELLA_H

#include "cellula.h"
#include <stddef.h>
#include "cellae/fixum.h"
#include "cellae/fixa/vacuum.h"
#include "cellae/fixa/saxum.h"
#include "cellae/fixa/murus.h"
#include "cellae/cibus.h"
#include "cellae/cibi/rapum.h"
#include "cellae/cibi/fungus.h"
#include "cellae/animus.h"
#include "cellae/animae/feles.h"
#include "cellae/animae/dalekus.h"
#include "cellae/animae/ursus.h"
#include "cellae/animae/corvus.h"
#include "cellae/deus.h"
#include "cellae/dei/zodus.h"
#include "cellae/dei/oculus.h"

/* cella plena in tabula — cellula basis + uniones phylorum et generum */
typedef struct cella {
    genus_t genus;
    int motum;
    int pondus;
    /* unio phylorum — basis phyli */
    union {
        fixum_t fixum;
        cibus_t cibus;
        animus_t animus;
        deus_t deus;
    } p;
    /* unio generum — data propria unius generis */
    union {
        vacuum_t vacuum;
        saxum_t saxum;
        murus_t murus;
        rapum_t rapum;
        fungus_t fungus;
        feles_t feles;
        dalekus_t dalekus;
        ursus_t ursus;
        corvus_t corvus;
        zodus_t zodus;
        oculus_t oculus;
    } g;
} cella_t;

/* declarationes anticipatae */
struct tabula;
struct fictio_vicinitas;

/* operationes unius generis cellulae */
typedef struct {
    const char *pictura;    /* glyphus, II columnas latus */
    char signum;            /* signum unum pro graticula oraculi */
    phylum_t phylum;
    unsigned int capacitates;  /* larva modorum permissorum (CAP_*) */
    void (*praepara)(struct cella *c);
    actio_t (*cogito)(const struct tabula *tab, int x, int y);
    /* fictio: genera actionem fictam pro probatione.
     * logica in plicis generum ipsis definitur. */
    void (*fictio)(const char *nomen, const struct fictio_vicinitas *vic,
                   char *actio, size_t mag);
} genus_ops_t;

/* tabula operationum — index per genus_t */
extern genus_ops_t genera_ops[GENERA_NUMERUS];

/* initia generum */
void vacuum_initia(void);
void saxum_initia(void);
void murus_initia(void);
void rapum_initia(void);
void fungus_initia(void);
void zodus_initia(void);
void oculus_initia(void);
void corvus_initia(void);

/*
 * cella_initia_ops — scribit pictura/signum in genus_ops[genus].
 * vocatur semel ab *_initia().
 */
void cella_initia_ops(genus_t genus, const char *pictura, char signum);

#endif /* CELLA_H */
