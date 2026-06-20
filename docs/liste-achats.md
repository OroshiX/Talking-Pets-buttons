# Liste d'achat France / UE

Les liens sont des exemples de composants compatibles. Il n'est pas obligatoire
d'acheter exactement ces references si les caracteristiques restent les memes.

## Electronique principale

- Arduino GIGA R1 WiFi:
  https://store.arduino.cc/products/giga-r1-wifi
- Module micro analogique a gain reglable, type MAX4466, compatible 3.3 V:
  https://www.adafruit.com/product/1063
- Cle USB-A courte, FAT32, 8 a 32 Go.
- Batterie externe USB-C 5 V, ou chargeur USB-C stable.
- Cable USB-C court pour alimenter le GIGA.

## Installation physique

- 6 a 8 boutons enregistrables independants.
- 2 tiles de yoga emboitables.
- Fil Dupont femelle-femelle ou cable 3 conducteurs pour le micro.
- Petite boite pour le GIGA.
- Petit mat ou support orientable pour le micro.
- Velcro adhesif, patins caoutchouc, double-face mousse fine.
- Filament PLA/PETG pour les supports 3D.

## Option bouton service

- 1 bouton poussoir normalement ouvert.
- 2 fils vers `D22` et `GND`.

## Application Android

- Application `ntfy`.
- Hotspot Android active pendant les sessions d'usage.
- Topic prive long et non devinable dans `secrets.ini`.

## A eviter

- Micro USB: le firmware V1 attend un signal analogique sur `A0`.
- Micro electret nu sans module d'amplification: le signal sera trop faible.
- Alimentation du micro en 5 V si sa sortie peut depasser 3.3 V.
