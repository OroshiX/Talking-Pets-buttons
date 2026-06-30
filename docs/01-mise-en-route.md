# Mise en route

## 1. Préparer le firmware

Dans CLion, VS Code ou un terminal PlatformIO :

```sh
pio run
pio run --target upload
```

La cible attendue est `giga_r1_m7` dans `platformio.ini`.

Pour éviter de retaper la vitesse du moniteur série à chaque fois, garder aussi
la ligne suivante dans `platformio.ini` :

```ini
monitor_speed = 115200
```

Ouvrir ensuite le moniteur série à `115200` bauds. Au démarrage, le GIGA affiche
la liste des commandes :

```text
help
list
status
usb
reload
select 0
ledtest 63
wifi
ntp
cal A1 12
cancel
testntfy
```

Ces commandes sont à taper dans le moniteur série connecté au GIGA, pas dans le
terminal de l'ordinateur. Elles sont l'interface de diagnostic et de calibration
du firmware.

`115200` bauds est la vitesse de communication entre l'ordinateur et le GIGA pour
ce moniteur série. Dans Arduino IDE, choisir `115200 baud` dans le menu du
moniteur série. Avec PlatformIO en terminal, utiliser :

```sh
pio device monitor -b 115200
```

## 2. Préparer la clé USB

Formater une clé USB-A en FAT32, puis copier la structure suivante :

```text
/config/buttons.csv
/config/settings.ini
/config/secrets.ini
/templates/
/logs/
/queue/
/captures/
```

Les exemples du dépôt sont dans `usb/config/`.

`secrets.ini` n'est pas versionné, car il contient le mot de passe hotspot et le
topic `ntfy`.

## 3. Brancher le micro

```text
Module micro analogique        Arduino GIGA

VCC  ------------------------>  3V3
GND  ------------------------>  GND
OUT  ------------------------>  A0
```

Utiliser `3V3`, pas `5V`. Garder les fils du micro courts : 30 cm maximum est un
bon objectif.

## 4. Placer le setup

Lire `docs/03-agencement.md`, puis installer :

- le GIGA derrière les tiles, hors zone de pattes ;
- le micro à 25-35 cm de haut, pointé vers le centre des boutons ;
- les boutons posés dans leurs supports 3D, avec l'évent le plus large orienté
  vers le micro.

## 5. Calibrer les boutons

Deux méthodes sont possibles : les commandes série ci-dessous, ou le panneau
physique décrit dans `docs/07-calibration.md`.

Pour chaque slot :

```text
cal A1 12
```

Presser ensuite le bouton A1 12 fois, depuis sa vraie position sur la tile.

Répéter :

```text
cal A2 12
cal A3 12
cal A4 12
cal B1 12
cal B2 12
```

Le GIGA crée les fichiers :

```text
/templates/A1.tpl
/templates/A2.tpl
...
```

## 6. Tester

Commandes utiles :

```text
status
list
usb
select 0
ledtest 63
wifi
ntp
testntfy
```

Puis presser chaque bouton 20 fois en conditions réelles. La checklist complète
est dans `docs/08-tests.md`.
