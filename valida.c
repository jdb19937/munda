/*
 * valida.c — instrumentum validationis schematum ex linea imperatoria
 *
 * Usus: ./valida schema.ison datum.isonl [datum2.isonl ...]
 *   validat quemque plicam ISONL contra schema.
 *   reddit 0 si omnia valida, 1 si errores inventi.
 */

#include "ison.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "usus: %s schema.ison datum.isonl [...]\n", argv[0]);
        return 2;
    }

    /* lege schema */
    schema_t schema;
    if (schema_lege_plicam(argv[1], &schema) < 0) {
        fprintf(stderr, "error: schema legi non potuit: %s\n", argv[1]);
        return 2;
    }

    if (schema.titulus[0])
        printf("schema: %s\n", schema.titulus);

    int errores_totales = 0;

    for (int i = 2; i < argc; i++) {
        char *isonl = ison_lege_plicam(argv[i]);
        if (!isonl) {
            fprintf(stderr, "error: plica legi non potuit: %s\n", argv[i]);
            errores_totales++;
            continue;
        }

        printf("valido: %s\n", argv[i]);
        int err = schema_valida_isonl(&schema, isonl);
        free(isonl);

        if (err == 0)
            printf("  validum est.\n");
        else
            errores_totales += err;
    }

    return errores_totales > 0 ? 1 : 0;
}
