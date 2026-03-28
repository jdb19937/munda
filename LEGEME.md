# Tabula Munda

Ludus simulationis tabularis in terminali, lingua C scriptus. Entia viva (feles, daleki, ursi) per oraculum LLM cogitant et agunt in tabula toroidali.

> **Nota:** README.md ignorandum est. Anglice scriptum est ad barbaros ineruditos pecunia emungendos.

## Aedificatio

```
make
```

Sine dependentiis externis (HTTPS per crispus). Binaria creantur:

- **curre** — cursus sine terminali
- **lude** — ludus interactivus cum terminali
- **fare** — imperativum oraculi (CLI ad LLM rogandum)
- **valida** — validator schematum ISON
- **daemonium** — servitor TCP, currit simulationem per retiam
- **coniunge** — cliens interactivus cum terminali (ut lude, per retiam)
- **specta** — cliens sine capite (ut curre, per retiam)
- **fac_certificatum** — genera par clavium EC P-256 pro daemonio

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
- `tabula.ison` — geometria, positiones, sapientum per genus
- `{phylum}/{genus}.isonl` — attributa individualia per cellam

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
| CORVUS      | ANIMA  | Corvus, avis callida et rapax       |
| ZODUS       | DEI    | Deus a lusore gubernatus            |
| OCULUS      | DEI    | Deus omnividens                     |

## Modi Actionum

QUIESCE, MOVE, PELLE, CAPE, TRAHE, LOQUERE, CLAMA, OPPUGNA.

## Daemonium

Servitor TCP qui simulationem currit et statum tabulae ad clientes transmittit.
Securitas per ECDHE P-256 + AES-128-GCM cum certificato ISON.

```
./fac_certificatum
./daemonium [munda [portus [tempus_ms]]]
./coniunge [-h hospes] [-p portus] [-c certificatum] [-g ZODUS|OCULUS]
./specta [-h hospes] [-p portus] [-c certificatum] [-g ZODUS|OCULUS] [-n gradus]
```

Quisque cliens conectus deum in tabula creat. Disconnexione deus deletur.

## Architectura

```
curre.c           — cursus sine terminali
lude.c            — ludus interactivus
daemonium.c       — servitor TCP retialis
coniunge.c        — cliens interactivus per retiam
specta.c          — cliens sine capite per retiam
fac_certificatum.c — generatio certificati EC P-256
tabula.c/h        — tabula toroidalis, gradus simulationis, onerator mundi
cella.h           — definitiones generum, phylorum, actionum
terminalis.c/h    — redditio terminalis (ANSI)
oraculum.c/h      — interfacies ad LLM (sync et async per crispus)
arcana/            — primitiva cryptographica (SHA-256, AES-GCM, EC P-256, bignum)
crispus/           — bibliotheca HTTPS (TLS 1.2, pendet ab arcana/)
retis/             — protocollum TCP bespoke (framing, ECDHE sessio, serialisatio)
cogitatio.c/h     — praecogitatio generica per oraculum
fare.c            — CLI imperativum oraculi
ison.c/h          — parser ISON, validator schematum
utilia.c/h        — utilia communia
valida.c          — instrumentum validationis schematum
cellae/            — implementationes singulorum generum
  fixa/            — vacuum, saxum, murus
  cibi/            — rapum, fungus
  animae/          — feles, dalekus, ursus, corvus
  dei/             — zodus, oculus
mundae/            — configurationes mundorum
schemae/           — schemata ISON pro validatione
oracula/           — provisores LLM (openai, anthropic, xai, fictus)
```

## Mundatio

```
make mundus
```
