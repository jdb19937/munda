# Tabula Munda

Zero-dependency C project implementing four standalone subsystems from scratch: a TLS 1.2 HTTPS client, a lightweight JSON parser with schema validation, a multi-provider LLM abstraction layer, and an agentic communication framework for coordinating autonomous agents on a shared grid.

Everything compiles with just make. No external libraries. POSIX only.

## crispus — From-Scratch HTTPS Replacing curl

A complete TLS 1.2 client library with all cryptography implemented internally, replacing libcurl with roughly three thousand lines of C. The crypto stack includes AES-128-GCM for authenticated encryption, elliptic curve Diffie-Hellman key exchange over P-256 with custom bignum arithmetic, RSA signature verification supporting up to RSA-4096, SHA-256 with HMAC for key derivation, and DER-format X.509 certificate parsing to extract RSA public keys.

It negotiates a single cipher suite, supports SNI, and handles chunked transfer-encoding. The API follows a synchronous easy-handle pattern alongside an async mode that forks per request with pipe-based IPC, supporting up to sixty-four concurrent requests without threading or shared state.

## json.c — Lightweight JSON Parser + Schema Validation

A single-file JSON library that performs no allocations for reading. It supports path-based queries with dotted notation and array indexing, incremental JSON construction, and JSONL iteration.

A standalone CLI validator checks JSONL data files against schemas following JSON Schema draft 2020-12 structure, validating required fields and type correctness and reporting per-line errors.

## Multi-Provider LLM Stack

A plugin architecture supporting multiple LLM providers behind a unified async interface. The provider is selected at runtime by model string prefix. Each provider implements three operations: formatting the provider-specific request body and headers, extracting text from the response, and parsing token usage metrics including input, output, cached, and reasoning tokens.

Supported providers include OpenAI via the Responses API, Anthropic via the Messages API with cache-aware token counting, xAI via Chat Completions, and a mock provider that delegates to per-type simulation functions for testing.

Requests flow through thirty-two async slots with non-blocking polling. Each slot tracks state transitions from free to in-flight to complete and accumulates per-model token statistics. Effort levels are forwarded to providers that support them.

## Agentic Grid Communication

Autonomous agents occupy cells on a toroidal grid. Each agent has local perception within a configurable radius of visibility, internal state including memory, satiety, and what it has heard, and communicates with neighbors through directed speech or broadcast to surrounding cells.

Agents accumulate what they hear and maintain a persistent thought state across turns. This creates emergent multi-turn dialogue where agents can coordinate, warn, argue, or deceive based on what they have heard and what they are thinking.

Multiple agent decisions can be collapsed into a single LLM call by sending one batchnar prompt containing each agent's local neighborhood and receiving all decisions in one response. Alternatively, the entire board state can be sent as an omninar query so the LLM can reason about team-wide strategy, deciding which agents should pursue food, which should attack, and which should call out enemy positions.

Batching parameters control the tradeoff between prompt size and decision quality, including maximum agents per batch, how many cycles to wait before sending a partial batch, and per-agent visibility radius. Actions are parsed back into typed structs and executed by the grid simulation engine with full physics including push chains, weight limits, attack thresholds, and toroidal wrapping.

## Build

Produces four binaries. Runs on macOS and Linux. Only needs a C compiler and POSIX headers.

## License

Free. Use however you like.
