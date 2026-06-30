# Calibration audio

La calibration crée un template moyen pour chaque bouton. Le firmware compare
ensuite chaque nouveau son à ces templates.

## Préparation

Avant de calibrer :

- mettre les boutons à leur place finale ;
- placer le micro à sa hauteur finale ;
- activer le hotspot Android ;
- brancher la clé USB.

Si tu calibres avec les commandes série, ouvrir aussi le moniteur série à
`115200` bauds.

Vérifier :

```text
status
list
select 0
```

## Calibrer avec le moniteur série

Commande :

```text
cal A1 12
```

Puis presser le bouton A1 12 fois. Attendre que le message audio soit fini avant
la pression suivante.

Le firmware affiche :

```text
Calibration A1: 1/12
Calibration A1: 2/12
...
Calibration complete.
```

Le fichier créé :

```text
/templates/A1.tpl
```

## Calibrer sans ordinateur

Le panneau physique permet de calibrer avec 3 boutons électriques et 6 LEDs :

- `CAL` sur `D22` : appui long pour entrer ou annuler la calibration ;
- `+` sur `D23` : sélectionner le bouton animal suivant ;
- `-` sur `D24` : sélectionner le bouton animal précédent ;
- LEDs `D25` à `D30` : affichage binaire du bouton sélectionné.

Les boutons `CAL`, `+` et `-` sont des boutons normalement ouverts branchés entre
la pin Arduino et `GND`. Ils n'ont pas besoin de résistances externes, car le
firmware utilise les pull-ups internes du GIGA.

Les LEDs s'affichent en binaire direct :

```text
LEDs de gauche à droite : 32 16 8 4 2 1
Pins correspondantes    : D30 D29 D28 D27 D26 D25

000000 = aucun bouton sélectionné
000001 = bouton n°1 dans buttons.csv
000010 = bouton n°2 dans buttons.csv
000011 = bouton n°3 dans buttons.csv
...
111111 = bouton n°63 maximum
```

Procédure :

1. utiliser `+` et `-` pour choisir le bouton animal à calibrer ;
2. vérifier que les LEDs ne sont pas toutes éteintes ;
3. faire un appui long sur `CAL` pendant environ 1 seconde ;
4. attendre les 3 clignotements de la LED intégrée ;
5. presser le bouton animal sélectionné 12 fois, ou le nombre configuré dans
   `calibration_samples` ;
6. à la fin, le template est sauvegardé et la sélection passe automatiquement au
   bouton suivant.

Si les 6 LEDs sont éteintes, la sélection vaut `0`. Dans cet état, un appui long
sur `CAL` ne lance pas de calibration et la LED intégrée clignote rapidement pour
indiquer l'erreur.

Pendant une calibration active, `+` et `-` sont ignorés pour ne pas mélanger les
exemples audio de deux boutons. Pour annuler, refaire un appui long sur `CAL`.

Depuis le moniteur série, la commande `select N` règle la même sélection :

```text
select 0   éteint les 6 LEDs binaires
select 1   sélectionne le bouton n°1
select 2   sélectionne le bouton n°2
```

`select` est ignorée pendant une calibration active.

## Calibrer tout le setup V1

```text
cal A1 12
cal A2 12
cal A3 12
cal A4 12
cal B1 12
cal B2 12
```

Pour 8 boutons, ajouter :

```text
cal B3 12
cal B4 12
```

## Quand refaire une calibration

Refaire le template si :

- le message du bouton est réenregistré ;
- le bouton change de support ;
- le micro change fortement de position ;
- la pièce de vacances est beaucoup plus réverbérante ou bruyante ;
- deux boutons sont souvent confondus.

## Réglages utiles

Dans `/config/settings.ini` :

```ini
confidence_threshold=0.75
match_distance_at_zero=0.12
min_trigger_abs=650
trigger_multiplier=4.0
```

Si le GIGA reconnaît trop facilement des bruits :

- monter `confidence_threshold` ;
- monter `trigger_multiplier` ;
- monter `min_trigger_abs`.

Si le GIGA rate des vrais boutons :

- rapprocher le micro ;
- augmenter le gain du micro ;
- baisser légèrement `min_trigger_abs` ;
- refaire la calibration avec le placement final.
