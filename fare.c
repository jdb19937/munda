/*
 * fare.c — imperativum oraculi
 *
 * Usus: ./fare [-s instructiones] rogatum ...
 *
 * MUNDA_SAPIENTIA=openai/gpt-5.4+high ./fare dic mihi fabulam
 * MUNDA_SAPIENTIA=anthropic/claude-sonnet-4-6 ./fare -s "responde Latine" salve
 * MUNDA_SAPIENTIA=xai/grok-3 ./fare quid est vita
 */

#include "oraculum.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static char *coniunge(int argc, char **argv, int ab)
{
    size_t lon = 0;
    for (int i = ab; i < argc; i++)
        lon += strlen(argv[i]) + 1;
    if (lon == 0) return strdup("");

    char *buf = malloc(lon);
    if (!buf) return NULL;
    char *p = buf;
    for (int i = ab; i < argc; i++) {
        if (i > ab) *p++ = ' ';
        size_t n = strlen(argv[i]);
        memcpy(p, argv[i], n);
        p += n;
    }
    *p = '\0';
    return buf;
}

int main(int argc, char **argv)
{
    const char *instructiones = NULL;
    int arg_initium = 1;

    /* lege -s instructiones */
    if (argc >= 3 && strcmp(argv[1], "-s") == 0) {
        instructiones = argv[2];
        arg_initium = 3;
    }

    if (arg_initium >= argc) {
        fprintf(stderr,
            "usus: %s [-s instructiones] rogatum ...\n"
            "\n"
            "MUNDA_SAPIENTIA forma: [provisor/]sapientum[+effort]\n"
            "  provisores: openai, anthropic, xai\n"
            "  effort: low, medium, high\n"
            "\n"
            "exempla:\n"
            "  MUNDA_SAPIENTIA=gpt-5.4 %s salve\n"
            "  MUNDA_SAPIENTIA=openai/gpt-5.4+high %s -s 'responde Latine' quid est\n"
            "  MUNDA_SAPIENTIA=anthropic/claude-sonnet-4-6 %s dic fabulam\n"
            "  MUNDA_SAPIENTIA=xai/grok-3 %s quid est vita\n",
            argv[0], argv[0], argv[0], argv[0], argv[0]);
        return 1;
    }

    char *rogatum = coniunge(argc, argv, arg_initium);
    if (!rogatum) {
        fprintf(stderr, "memoria defecit\n");
        return 1;
    }

    srand((unsigned)time(NULL));
    oraculum_initia();

    char *resp = NULL;
    int rc = oraculum_roga(NULL, instructiones, rogatum, &resp);

    if (rc == 0 && resp)
        printf("%s\n", resp);
    else
        fprintf(stderr, "error: %s\n", resp ? resp : "ignotus");

    free(resp);
    free(rogatum);
    oraculum_fini();

    return rc == 0 ? 0 : 1;
}
