# Câblage

La V1 audio-only n'utilise plus de câblage bouton par bouton. Les boutons
enregistrables restent autonomes.

Le seul câblage obligatoire est celui du micro analogique :

```text
Module micro analogique        Arduino GIGA

VCC  ------------------------>  3V3
GND  ------------------------>  GND
OUT  ------------------------>  A0
```

Le reste :

```text
Clé USB FAT32  ----> USB-A du GIGA
Alimentation   ----> USB-C du GIGA
Hotspot Android ))) Wi-Fi ))) GIGA
```

## Panneau de calibration optionnel

Le panneau permet de sélectionner et calibrer les boutons animaux sans ordinateur.
Il n'est pas obligatoire : les commandes série restent disponibles.

Boutons électriques normalement ouverts :

```text
Bouton CAL  ----> D22 et GND
Bouton +    ----> D23 et GND
Bouton -    ----> D24 et GND
```

Ces 3 boutons n'ont pas besoin de résistances externes. Le firmware configure les
pins en `INPUT_PULLUP`, donc le bouton relie simplement la pin à `GND` quand on
appuie.

LEDs binaires :

```text
LED bit 1   ----> D25 -> résistance 1 kΩ -> LED -> GND
LED bit 2   ----> D26 -> résistance 1 kΩ -> LED -> GND
LED bit 4   ----> D27 -> résistance 1 kΩ -> LED -> GND
LED bit 8   ----> D28 -> résistance 1 kΩ -> LED -> GND
LED bit 16  ----> D29 -> résistance 1 kΩ -> LED -> GND
LED bit 32  ----> D30 -> résistance 1 kΩ -> LED -> GND
```

Chaque LED a besoin de sa propre résistance, car une LED est une sortie pilotée
par l'Arduino. Les pull-ups internes ne protègent pas les LEDs.

Ordre visuel recommandé de gauche à droite :

```text
32 16 8 4 2 1
D30 D29 D28 D27 D26 D25
```
