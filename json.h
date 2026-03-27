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

#endif /* JSON_H */
