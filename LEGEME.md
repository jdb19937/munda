# Munda

Ludus simulationis tabularis in terminali, lingua C scriptus. Entia viva (feles, daleki, ursi) per oraculum LLM cogitant et agunt in tabula toroidali.

## Aedificatio

```
make
```

Requiritur `libcurl`. Duo binaria creantur:

- **munda** — ludus ipse
- **fare** — imperativum oraculi (CLI ad LLM rogandum)

## Usus

```
./munda [--obsidium] [latus [tempus_ms]]
```

| Argumentum    | Descriptio                                | Praefinitum |
|---------------|-------------------------------------------|-------------|
| `--obsidium`  | Modus obsidii: daleki ursum circumdant  | —           |
| `latus`       | Longitudo lateris tabulae (4–128)         | 20          |
| `tempus_ms`   | Intervallum inter gradus in ms            | 100         |

Claves in ludo:
- **Sagittae** — guberna Zodum (deum lusoris)
- **q** — exi
- **Ctrl-L** — repinge

## Fare (Imperativum Oraculi)

```
MUNDA_SAPIENTIA=openai/gpt-5.4 ./fare dic mihi fabulam
MUNDA_SAPIENTIA=anthropic/claude-sonnet-4-6 ./fare -s "responde Latine" salve
MUNDA_SAPIENTIA=xai/grok-3 ./fare quid est vita
```

Forma: `[provisor/]sapientum[+effort]` ubi effort = `low`, `medium`, `high`.

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

## Modi Actionum

QUIESCE, MOVE, PELLE, CAPE, TRAHE, LOQUERE, CLAMA, OPPUGNA.

## Architectura

```
main.c            — ansa principalis, initia, implendi tabula
tabula.c/h        — tabula toroidalis, gradus simulationis
cella.h           — definitiones generum, phylorum, actionum
terminalis.c/h    — redditio terminalis (ANSI)
oraculum.c/h      — interfacies ad LLM (sync et async per libcurl)
cogitatio.c/h     — praecogitatio generica per oraculum
fare.c            — CLI imperativum oraculi
json.c/h          — parser JSON
utilia.c/h        — utilia communia
cellae/            — implementationes singulorum generum
  fixa/            — vacuum, saxum, murus
  cibi/            — rapum, fungus
  animae/          — feles, dalekus, ursus
  dei/             — zodus
oracula/           — provisores LLM (openai, anthropic, xai)
```

## Variabiles Ambientis

| Variabilis         | Descriptio                              |
|--------------------|-----------------------------------------|
| `MUNDA_SAPIENTIA`  | Exemplar LLM (e.g. `openai/gpt-5.4`)  |

## Mundatio

```
make mundus
```
