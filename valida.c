/*
 * valida.c — instrumentum validationis schematum ex linea imperatoria
 *
 * Usus: ./valida schema.json datum.jsonl [datum2.jsonl ...]
 *   validat quemque fasciculum JSONL contra schema.
 *   reddit 0 si omnia valida, 1 si errores inventi.
 */

#include "json.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "usus: %s schema.json datum.jsonl [...]\n", argv[0]);
        return 2;
    }

    /* lege schema */
    schema_t schema;
    if (schema_lege_fasciculum(argv[1], &schema) < 0) {
        fprintf(stderr, "error: schema legi non potuit: %s\n", argv[1]);
        return 2;
    }

    if (schema.titulus[0])
        printf("schema: %s\n", schema.titulus);

    int errores_totales = 0;

    for (int i = 2; i < argc; i++) {
        char *jsonl = json_lege_fasciculum(argv[i]);
        if (!jsonl) {
            fprintf(stderr, "error: fasciculus legi non potuit: %s\n", argv[i]);
            errores_totales++;
            continue;
        }

        printf("valido: %s\n", argv[i]);
        int err = schema_valida_jsonl(&schema, jsonl);
        free(jsonl);

        if (err == 0)
            printf("  validum est.\n");
        else
            errores_totales += err;
    }

    return errores_totales > 0 ? 1 : 0;
}
