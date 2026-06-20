# Mise en route

## 1. Preparer le firmware

Dans CLion, VS Code ou un terminal PlatformIO:

```sh
pio run
pio run --target upload
```

La cible attendue est `giga_r1_m7` dans `platformio.ini`.

Ouvrir ensuite le moniteur serie a `115200` bauds. Au demarrage, le GIGA affiche
la liste des commandes:

```text
help
list
status
reload
cal A1 12
cancel
testntfy
```

## 2. Preparer la cle USB

Formater une cle USB-A en FAT32, puis copier la structure suivante:

```text
/config/buttons.csv
/config/settings.ini
/config/secrets.ini
/templates/
/logs/
/queue/
/captures/
```

Les exemples du depot sont dans `usb/config/`.

`secrets.ini` n'est pas versionne, car il contient le mot de passe hotspot et le
topic `ntfy`.

## 3. Brancher le micro

```text
Module micro analogique        Arduino GIGA

VCC  ------------------------>  3V3
GND  ------------------------>  GND
OUT  ------------------------>  A0
```

Utiliser `3V3`, pas `5V`. Garder les fils du micro courts: 30 cm maximum est un
bon objectif.

## 4. Placer le setup

Lire `docs/agencement.md`, puis installer:

- le GIGA derriere les tiles, hors zone de pattes;
- le micro a 25-35 cm de haut, pointe vers le centre des boutons;
- les boutons dans leurs supports 3D, event sonore oriente vers le micro.

## 5. Calibrer les boutons

Pour chaque slot:

```text
cal A1 12
```

Presser ensuite le bouton A1 12 fois, depuis sa vraie position sur la tile.

Repeter:

```text
cal A2 12
cal A3 12
cal A4 12
cal B1 12
cal B2 12
```

Le GIGA cree les fichiers:

```text
/templates/A1.tpl
/templates/A2.tpl
...
```

## 6. Tester

Commandes utiles:

```text
status
list
testntfy
```

Puis presser chaque bouton 20 fois en conditions reelles. La checklist complete
est dans `docs/tests.md`.
