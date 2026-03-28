# Munda

Ludus simulationis tabularis in terminali, lingua C scriptus. Entia viva (feles, daleki, ursi) per oraculum LLM cogitant et agunt in tabula toroidali.

## Aedificatio

```
make
```

Sine dependentiis externis (HTTPS per crispus). Binaria creantur:

- **curre** — cursus sine terminali
- **lude** — ludus interactivus cum terminali
- **fare** — imperativum oraculi (CLI ad LLM rogandum)
- **valida** — validator schematum JSON

## Usus

```
./lude [munda [tempus_ms]]
./curre [munda [gradus]]
```

| Argumentum    | Descriptio                                | Praefinitum         |
|---------------|-------------------------------------------|---------------------|
| `munda`       | Via ad directorium mundi                  | mundae/imperium     |
| `tempus_ms`   | Intervallum inter gradus in ms (lude)     | 100                 |
| `gradus`      | Numerus graduum simulationis (curre)      | 20                  |

Claves in ludo:
- **Sagittae** — guberna Zodum (deum lusoris)
- **q** — exi
- **Ctrl-L** — repinge

## Fare (Imperativum Oraculi)

```
./fare -m openai/gpt-5.4 dic mihi fabulam
./fare -m anthropic/claude-sonnet-4-6 -s "responde Latine" salve
./fare -m xai/grok-3 quid est vita
```

Forma: `-m [provisor/]sapientum[+effort]` ubi effort = `low`, `medium`, `high`.

## Mundae (Configurationes Mundorum)

Quisque mundus est directorium cum:
- `tabula.json` — geometria, positiones, sapientum per genus
- `{phylum}/{genus}.jsonl` — attributa individualia per cellam

```
mundae/
  imperium/         — mundus plenus cum Zodo
  obsidium/         — ursus ab X dalekis circumdatus
```

## Genera Cellularum

| Genus       | Phylum | Descriptio                          |
|-------------|--------|-------------------------------------|
| VACUUM      | FIXUM  | Spatium vacuum                      |
| SAXUM       | FIXUM  | Lapis mobilis                       |
| MURUS       | FIXUM  | Murus immobilis                     |
| RAPUM       | CIBUS  | Rapum — satietas +1                 |
| FUNGUS      | CIBUS  | Fungus — satietas +2                |
| FELES       | ANIMA  | Feles errans                        |
| DALEKUS     | ANIMA  | Bot quod oraculum LLM groupnar rogat |
| URSUS       | ANIMA  | Ursus venator                       |
| ZODUS       | DEI    | Deus a lusore gubernatus            |
| OCULUS      | DEI    | Deus omnividens                     |

## Modi Actionum

QUIESCE, MOVE, PELLE, CAPE, TRAHE, LOQUERE, CLAMA, OPPUGNA.

## Architectura

```
curre.c           — cursus sine terminali
lude.c            — ludus interactivus
tabula.c/h        — tabula toroidalis, gradus simulationis, onerator mundi
cella.h           — definitiones generum, phylorum, actionum
terminalis.c/h    — redditio terminalis (ANSI)
oraculum.c/h      — interfacies ad LLM (sync et async per crispus)
crispus/           — bibliotheca HTTPS (TLS 1.2, AES-GCM, ECDHE, SHA-256)
cogitatio.c/h     — praecogitatio generica per oraculum
fare.c            — CLI imperativum oraculi
json.c/h          — parser JSON, validator schematum
utilia.c/h        — utilia communia
valida.c          — instrumentum validationis schematum
cellae/            — implementationes singulorum generum
  fixa/            — vacuum, saxum, murus
  cibi/            — rapum, fungus
  animae/          — feles, dalekus, ursus
  dei/             — zodus, oculus
mundae/            — configurationes mundorum
schemae/           — schemata JSON pro validatione
oracula/           — provisores LLM (openai, anthropic, xai, fictus, munda)
```

## Mundatio

```
make mundus
```
