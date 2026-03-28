# Technical Overview

Zero-dependency C project implementing four standalone subsystems from scratch: a TLS 1.2 HTTPS client, a lightweight JSON parser with schema validation, a multi-provider LLM abstraction layer, and an agentic communication framework for coordinating autonomous agents on a shared grid via groupnar batch queries.

Everything compiles with just `make`. No external libraries. POSIX only.

## crispus — From-Scratch HTTPS Replacing curl

A complete TLS 1.2 client library with all cryptography implemented internally. Replaces libcurl with ~3,000 lines of C.

**Crypto stack (all from scratch):**
- **AES-128-GCM** — FIPS 197 block cipher + NIST SP 800-38D authenticated encryption
- **ECDHE over P-256** — Elliptic curve Diffie-Hellman key exchange with custom bignum arithmetic
- **RSA signature verification** — PKCS#1 v1.5 with SHA-256, supports up to RSA-4096
- **SHA-256 + HMAC** — FIPS 180-4 digest, used for TLS PRF key derivation
- **X.509/ASN.1 parsing** — DER certificate parsing to extract RSA public keys

Single cipher suite: `TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256`. SNI support. Chunked transfer-encoding.

**API mirrors curl's easy/multi pattern:**

```c
// synchronous
CRISPUS *h = crispus_facilis_initia();
crispus_facilis_pone(h, CRISPUSOPT_URL, "https://api.example.com/v1");
crispus_facilis_pone(h, CRISPUSOPT_CAMPI_POSTAE, json_body);
crispus_facilis_pone(h, CRISPUSOPT_FUNCTIO_SCRIBENDI, callback);
crispus_facilis_age(h);

// async (fork-per-request, up to 64 concurrent)
CRISPUSM *m = crispus_multi_initia();
crispus_multi_adde(m, handle1);
crispus_multi_adde(m, handle2);
crispus_multi_age(m, &running);  // non-blocking poll
```

Async concurrency uses `fork()` per request with pipe-based IPC — simpler than threading, no shared state.

## json.c — Lightweight JSON Parser + Schema Validation

Single-file JSON library. No allocations for reading. Path-based queries with dotted notation and array indexing.

```c
// navigate nested structures
char *val  = json_da_chordam(json, "choices[0].message.content");
double num = json_da_numerum(json, "usage.input_tokens");
char *raw  = json_da_crudum(json, "output[0].content");

// build JSON incrementally
json_scriptor_t *s = json_scriptor_crea();
json_scriptor_adde(s, "role", "user");
json_scriptor_adde_crudum(s, "count", "42");
char *result = json_scriptor_fini(s);

// iterate JSONL
json_pro_quaque_linea(jsonl_data, per_line_callback, ctx);
```

**Schema validation** with a standalone CLI validator:

```sh
./valida schemae/animae/ursus-schema.json mundae/imperium/animae/ursus.jsonl
```

Validates required fields, type checking (string vs number), and reports per-line errors. Schemas use JSON Schema draft 2020-12 structure.

## Multi-Provider LLM Stack

Plugin architecture supporting multiple LLM providers behind a unified async interface. Provider selected at runtime by model string: `"openai/gpt-5.4+high"`, `"anthropic/claude-sonnet-4-6"`, `"xai/grok-3"`.

**Provider interface (3 functions each):**

| Operation | Purpose |
|-----------|---------|
| `para()` | Format provider-specific JSON request body + HTTP headers |
| `extrahe()` | Extract text from provider-specific response format |
| `signa()` | Parse token usage metrics (input, output, cached, reasoning) |

**Supported providers:**
- **OpenAI** — Responses API (`output[0].content[0].text`)
- **Anthropic** — Messages API (`content[0].text`), cache-aware token counting
- **xAI** — Chat Completions (`choices[0].message.content`)
- **Local deterministic** — Rule-based fallback, no HTTP
- **Mock** — Delegates to per-type simulation functions for testing

Requests flow through 32 async slots with non-blocking polling. Each slot tracks state transitions (free → in-flight → complete) and accumulates per-model token statistics.

```c
int slot = oraculum_mitte(model, system_prompt, user_json);
oraculum_processus();  // poll all in-flight
char *response = oraculum_status(slot, &rc);
```

Effort levels (`+low`, `+medium`, `+high`) are forwarded to providers that support them.

## Agentic Grid Communication

Autonomous agents occupy cells on a toroidal grid. Each agent has local perception (a configurable radius of visibility), internal state (memory, satiety, what it's heard), and communicates with neighbors through speech acts.

**Communication primitives:**
- `LOQUERE` — Directed speech to one adjacent neighbor
- `CLAMA` — Broadcast to all 8 surrounding cells

Agents accumulate what they hear in an `audita` buffer. Their internal `mens` (thought state) persists across turns. This creates emergent multi-turn dialogue — agents can coordinate, warn, argue, or deceive based on what they've heard and what they're thinking.

**Groupnar batch queries** collapse multiple agent decisions into a single LLM call:

```json
// one prompt, multiple agents
{
  "U001": {"vicinitas": [[".",".","."],[".",  "@","F"],[".","r","."]], "satietas": 5, "audita": "[B003]: ursus ad meridies!"},
  "U002": {"vicinitas": [["W",".","."],["#","@","."],[".",".",  "."]], "satietas": 3},
  "U003": {"vicinitas": [[".","F","."],[".",  "@","."],[".",".",  "r"]], "satietas": 1}
}

// one response, all decisions
{"U001": "oppugna oriens", "U002": "pelle septentrio", "U003": "cape meridies"}
```

A second mode sends the entire board state so the LLM can reason about team-wide strategy — which agents should pursue food, which should attack, which should call out enemy positions.

**Batching parameters control the tradeoff:**
- `fasciculus_mag` — Max agents per batch (prompt size vs decision quality)
- `patientia` — How many cycles to wait for a full batch before sending partial
- `radius` — Per-agent visibility (2-5 cells), determines neighborhood JSON size

Actions are parsed back into typed structs with mode, direction, speech content, and thought updates — then executed by the grid simulation engine with full physics (push chains, weight limits, attack thresholds, toroidal wrapping).

## Build

```
make
```

Produces four binaries. Runs on macOS and Linux. Only needs a C compiler and POSIX headers.
