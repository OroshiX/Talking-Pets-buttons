# Calibration audio

La calibration cree un template moyen pour chaque bouton. Le firmware compare
ensuite chaque nouveau son a ces templates.

## Preparation

Avant de calibrer:

- mettre les boutons a leur place finale;
- placer le micro a sa hauteur finale;
- activer le hotspot Android;
- brancher la cle USB;
- ouvrir le moniteur serie a `115200` bauds.

Verifier:

```text
status
list
```

## Calibrer un bouton

Commande:

```text
cal A1 12
```

Puis presser le bouton A1 12 fois. Attendre que le message audio soit fini avant
la pression suivante.

Le firmware affiche:

```text
Calibration A1: 1/12
Calibration A1: 2/12
...
Calibration complete.
```

Le fichier cree:

```text
/templates/A1.tpl
```

## Calibrer tout le setup V1

```text
cal A1 12
cal A2 12
cal A3 12
cal A4 12
cal B1 12
cal B2 12
```

Pour 8 boutons, ajouter:

```text
cal B3 12
cal B4 12
```

## Quand refaire une calibration

Refaire le template si:

- le message du bouton est reenregistre;
- le bouton change de support;
- le micro change fortement de position;
- la piece de vacances est beaucoup plus reverberante ou bruyante;
- deux boutons sont souvent confondus.

## Reglages utiles

Dans `/config/settings.ini`:

```ini
confidence_threshold=0.75
match_distance_at_zero=0.12
min_trigger_abs=650
trigger_multiplier=4.0
```

Si le GIGA reconnait trop facilement des bruits:

- monter `confidence_threshold`;
- monter `trigger_multiplier`;
- monter `min_trigger_abs`.

Si le GIGA rate des vrais boutons:

- rapprocher le micro;
- augmenter le gain du micro;
- baisser legerement `min_trigger_abs`;
- refaire la calibration avec le placement final.
