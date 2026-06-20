# Liste d'achat France / UE

Les liens sont des exemples de composants compatibles. Il n'est pas obligatoire
d'acheter exactement ces références si les caractéristiques restent les mêmes.

## Électronique principale

- Arduino GIGA R1 WiFi :
  https://store.arduino.cc/products/giga-r1-wifi
- Module micro analogique à gain réglable, type MAX4466, compatible 3.3 V :
  https://www.adafruit.com/product/1063
- Clé USB-A courte, FAT32, 8 à 32 Go.
- Batterie externe USB-C 5 V, ou chargeur USB-C stable.
- Câble USB-C court pour alimenter le GIGA.

## Installation physique

- 6 à 8 boutons enregistrables indépendants.
- 2 tiles de yoga emboîtables.
- Fil Dupont femelle-femelle ou câble 3 conducteurs pour le micro.
- Petite boîte pour le GIGA.
- Petit mât ou support orientable pour le micro.
- Velcro adhésif, patins caoutchouc, double-face mousse fine.
- Filament PLA/PETG pour les supports 3D.

## Option bouton service

- 1 bouton poussoir normalement ouvert.
- 2 fils vers `D22` et `GND`.

## Application Android

- Application `ntfy`.
- Hotspot Android activé pendant les sessions d'usage.
- Topic privé long et non devinable dans `secrets.ini`.

## À éviter

- Micro USB : le firmware V1 attend un signal analogique sur `A0`.
- Micro électret nu sans module d'amplification : le signal sera trop faible.
- Alimentation du micro en 5 V si sa sortie peut dépasser 3.3 V.
