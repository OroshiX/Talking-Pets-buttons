# Agencement physique

## Vue de dessus

![Agencement audio](images/audio-layout.svg)

```text
Mur / zone hors passage animal

        [Boitier GIGA]
             |
        mat micro 25-35 cm haut
             v
        micro pointe vers centre des boutons

+--------------------+--------------------+
| Tile A             | Tile B             |
|                    |                    |
|  A1      A2        |  B1      B2        |
|                    |                    |
|  A3      A4        |  B3      B4        |
|                    |                    |
+--------------------+--------------------+

Animal arrive plutot par le bas / cote ouvert.
```

## Positions recommandées

- Boitier GIGA : derriere les tiles, hors zone de pattes.
- Micro: environ 20 cm derriere le bord arriere des tiles.
- Hauteur micro: 25 a 35 cm au-dessus du sol.
- Orientation micro : pointe vers le centre géométrique des boutons.
- Distance micro-boutons: 35 a 80 cm si possible, moins de 1 m en V1.
- Boutons : 8 à 12 cm d'espace entre eux.

## Orientation des boutons

Chaque bouton doit être dans un support imprime. L'évent le plus large du
support est orienté vers le micro.

```text
Micro
  ^
  |
  |       event large du support
  |              ^
  |              |
+---+        +--------+
| A1|        |   A2   |
+---+        +--------+
```

## Routine de voyage

Pour déplacer le setup :

1. laisser les boutons dans leurs supports ;
2. empiler les tiles ;
3. garder le micro fixe au boitier ou a un petit mat pliable ;
4. brancher le GIGA a une batterie externe ;
5. activer le hotspot Android;
6. attendre `Wi-Fi connected` et `NTP synced` dans le moniteur série si tu
   vérifies depuis l'ordinateur.

Si le placement change beaucoup, lancer au minimum un test de 5 pressions par
bouton. Si la confiance baisse nettement, refaire la calibration.
