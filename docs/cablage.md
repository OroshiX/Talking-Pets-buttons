# Cablage

La V1 audio-only n'utilise plus de cablage bouton par bouton. Les boutons
enregistrables restent autonomes.

Le seul cablage obligatoire est celui du micro analogique:

```text
Module micro analogique        Arduino GIGA

VCC  ------------------------>  3V3
GND  ------------------------>  GND
OUT  ------------------------>  A0
```

Le reste:

```text
Cle USB FAT32  ----> USB-A du GIGA
Alimentation   ----> USB-C du GIGA
Hotspot Android ))) Wi-Fi ))) GIGA
```

Le bouton service optionnel se branche entre `D22` et `GND`, mais il n'est pas
necessaire pour calibrer: la V1 utilise les commandes serie.
