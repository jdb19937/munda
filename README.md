# munda

**A terminal-based simulation where autonomous agents — cats, bears, daleks, crows — inhabit a toroidal grid, make decisions through LLM inference, engage in emergent multi-turn dialogue, and wage war under the gaze of player-controlled deities. Multiplayer over encrypted TCP. Zero external dependencies.**

## What This Is

munda is an agentic grid simulation. Entities live on a wrapping toroidal board, perceive their surroundings, and decide what to do by calling large language models. They move, push rocks, eat food, hunt each other, talk, shout, and fight — all driven by natural language reasoning that produces genuinely emergent behavior. Cats wander curiously. Bears hunt deliberately. Daleks exterminate methodically. Crows steal and scheme.

A player-controlled deity walks among them, issuing divine commands through the arrow keys. An omniscient observer deity watches everything unfold. Connect over the network and multiple deities can inhabit the same world simultaneously, each seeing the simulation through their own terminal.

The entire thing — HTTPS calls to LLM providers, JSON parsing, TLS encryption, the TCP multiplayer protocol — is built from scratch. No libcurl. No OpenSSL. No JSON library. The cryptographic handshake that secures your multiplayer session uses the same ECDHE P-256 and AES-128-GCM implementation that powers the HTTPS calls to the inference APIs.

## Providers

Multi-provider LLM support with batched async inference:

- **OpenAI** — GPT models
- **Anthropic** — Claude models
- **xAI** — Grok models

Each entity type can use a different model. Configurable per-world via JSON.

## Entity Types

| Entity | Behavior |
|---|---|
| **Feles** | Curious wanderer |
| **Ursus** | Deliberate hunter |
| **Dalekus** | Methodical exterminator |
| **Corvus** | Cunning scavenger and thief |
| **Zodus** | Player-controlled deity |
| **Oculus** | Omniscient observer deity |

The grid also contains walls, pushable rocks, and two kinds of food (turnips and mushrooms) that restore satiety.

## Building

```bash
face
```

Produces eight binaries: `curre`, `lude`, `fare`, `valida`, `daemonium`, `coniunge`, `specta`, and `fac_certificatum`.

## Playing

```bash
./lude [world [interval_ms]]
```

Arrow keys move your deity. Agents think and act each turn. `q` to quit.

For headless simulation:

```bash
./curre [world [steps]]
```

## fare — LLM Command Line

A standalone CLI for querying any supported LLM provider:

```bash
./fare -m openai/gpt-4o tell me a story
./fare -m anthropic/claude-sonnet-4-6 -s "respond briefly" what is life
./fare -m xai/grok-3 explain quantum entanglement
```

## Networked Multiplayer

Generate a certificate, start the daemon, and connect:

```bash
./fac_certificatum
./daemonium [world [port [interval_ms]]]
./coniunge [-h host] [-p port] [-c cert] [-g ZODUS|OCULUS]
```

Each connected client spawns a deity on the board. Disconnect and the deity vanishes. The daemon runs the simulation; clients render their view of it. The TCP protocol uses a custom framing layer with ECDHE P-256 session keys and AES-128-GCM authenticated encryption.

For headless networked observation:

```bash
./specta [-h host] [-p port] [-c cert] [-g ZODUS|OCULUS] [-n steps]
```

## World Configuration

Each world is a directory containing a `tabula.ison` (geometry, positions, model assignments) and per-entity JSONL attribute files:

```text
mundae/
  imperium/         — full world with Zodus
  obsidium/         — a bear surrounded by daleks
```

## License

Free. Public domain. Use however you like.
