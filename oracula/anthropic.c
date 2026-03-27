/*
 * oracula/anthropic.c — provisor Anthropic (Messages API)
 */

#include "provisor.h"
#include "../json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int para(const char *nomen, const char *conatus,
                const char *clavis_api,
                const char *instructiones, const char *rogatum,
                char **corpus, struct curl_slist **capita)
{
    char *eff_user = json_effuge(rogatum);
    if (!eff_user) return -1;

    char *eff_sys = NULL;
    if (instructiones) {
        eff_sys = json_effuge(instructiones);
        if (!eff_sys) { free(eff_user); return -1; }
    }

    size_t mag = strlen(eff_user) + strlen(nomen) + 512;
    if (eff_sys) mag += strlen(eff_sys);

    char *buf = malloc(mag);
    if (!buf) { free(eff_user); free(eff_sys); return -1; }

    char *p = buf;
    p += sprintf(p, "{\"model\":\"%s\",\"max_tokens\":1024", nomen);

    if (conatus[0] && strcmp(conatus, "low") != 0)
        p += sprintf(p, ",\"thinking\":{\"type\":\"enabled\","
                        "\"budget_tokens\":4096}");

    if (eff_sys)
        p += sprintf(p, ",\"system\":\"%s\"", eff_sys);

    p += sprintf(p, ",\"messages\":[{\"role\":\"user\","
                    "\"content\":\"%s\"}]}", eff_user);

    free(eff_user);
    free(eff_sys);

    char caput_auth[512];
    snprintf(caput_auth, sizeof(caput_auth), "x-api-key: %s", clavis_api);

    struct curl_slist *c = NULL;
    c = curl_slist_append(c, "Content-Type: application/json");
    c = curl_slist_append(c, caput_auth);
    c = curl_slist_append(c, "anthropic-version: 2023-06-01");

    *corpus = buf;
    *capita = c;
    return 0;
}

static char *extrahe(const char *json)
{
    char *textus = json_da_chordam(json, "content[0].text");
    if (textus) return textus;

    char *error = json_da_chordam(json, "error.message");
    if (error) return error;

    return strdup(json);
}

static void signa(const char *json, long *accepta, long *recondita,
                   long *emissa, long *cogitata)
{
    *accepta   = json_da_numerum(json, "usage.input_tokens");
    *emissa    = json_da_numerum(json, "usage.output_tokens");
    *recondita = json_da_numerum(json, "usage.cache_read_input_tokens");
    /* Anthropic: cogitata intra emissa continentur, sed non separatim dantur */
    *cogitata  = 0;
}

const provisor_t provisor_anthropic = {
    .nomen      = "anthropic",
    .clavis_env = "ANTHROPIC_API_KEY",
    .finis_url  = "https://api.anthropic.com/v1/messages",
    .para       = para,
    .extrahe    = extrahe,
    .signa      = signa
};
