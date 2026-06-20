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

Le bouton service optionnel se branche entre `D22` et `GND`, mais il n'est pas
nécessaire pour calibrer : la V1 utilise les commandes série.
