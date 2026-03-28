/*
 * json.h — JSON auxiliaria
 *
 * Scriptor aedificat objecta JSON.
 * Lector paria clavis-valor ex objecto plano extrahit.
 * Navigator per viam punctatam ("a.b[0].c") navigat.
 */

#ifndef JSON_H
#define JSON_H

#include <stddef.h>

/* --- scriptor --- */

typedef struct json_scriptor json_scriptor_t;

json_scriptor_t *json_scriptor_crea(void);
void json_scriptor_adde(json_scriptor_t *js, const char *clavis,
                        const char *valor);
void json_scriptor_adde_crudum(json_scriptor_t *js, const char *clavis,
                               const char *valor);
char *json_scriptor_fini(json_scriptor_t *js);

/* --- lector parium (objectum planum) --- */

typedef struct json_par {
    char clavis[64];
    char valor[64];
} json_par_t;

int json_lege(const char *json, json_par_t *pares, int max_pares);

/* --- effugere --- */

char *json_effuge(const char *textus);

/* --- navigator per viam --- */

/*
 * naviga JSON per viam punctatam et extrahe valorem.
 * via exempla: "usage.input_tokens", "output[0].content[0].text",
 *              "choices[0].message.content", "error.message"
 *
 * json_da_chordam: reddit chordam allocatam (vocans liberet), vel NULL.
 * json_da_numerum: reddit numerum, vel 0 si non inventum.
 */
char *json_da_chordam(const char *json, const char *via);
long json_da_numerum(const char *json, const char *via);

/*
 * json_da_crudum: reddit valorem crudum (objectum, array, chordam, numerum)
 * ut chordam allocatam. vocans liberet per free().
 */
char *json_da_crudum(const char *json, const char *via);

/*
 * json_claves: extrahe claves primi gradus objecti JSON.
 * scribit in tabulam clavium, reddit numerum clavium.
 */
int json_claves(const char *json, char claves[][64], int max);

/* --- fasciculi --- */

/*
 * json_lege_fasciculum — legit fasciculum integrum in memoriam.
 * vocans liberet per free(). reddit NULL si error.
 */
char *json_lege_fasciculum(const char *via);

/* --- JSONL --- */

/*
 * json_pro_quaque_linea — iterat per lineas JSONL.
 * pro quaque linea non vacua, legit paria et vocat functorem.
 * reddit numerum linearum processarum.
 */
typedef void (*json_linea_functor_t)(const json_par_t *pp, int n, void *ctx);
int json_pro_quaque_linea(const char *jsonl, json_linea_functor_t f, void *ctx);

/* --- schema --- */

#define SCHEMA_CAMPI_MAX 32

/* typus campi */
typedef enum {
    TYPUS_CHORDA = 0,
    TYPUS_NUMERUS
} typus_t;

/* unus campus schematis */
typedef struct {
    char nomen[64];
    typus_t typus;
    int necessarium;        /* 1 si in "required" */
} schema_campus_t;

/* schema integrum */
typedef struct {
    char titulus[128];
    schema_campus_t campi[SCHEMA_CAMPI_MAX];
    int num_campi;
} schema_t;

/*
 * schema_lege — legit schema ex chorda JSON.
 * reddit 0 si bene, -1 si error.
 */
int schema_lege(const char *json, schema_t *s);

/*
 * schema_lege_fasciculum — legit schema ex fasciculo.
 * reddit 0 si bene, -1 si error.
 */
int schema_lege_fasciculum(const char *via, schema_t *s);

/*
 * schema_valida — validat paria contra schema.
 * reddit 0 si validum. si invalidum, scribit errorem in buf et reddit -1.
 */
int schema_valida(const schema_t *s, const json_par_t *pp, int n,
                  char *error, size_t mag);

/*
 * schema_valida_jsonl — validat fasciculum JSONL contra schema.
 * scribit errores ad stderr. reddit numerum errorum.
 */
int schema_valida_jsonl(const schema_t *s, const char *jsonl);

#endif /* JSON_H */
