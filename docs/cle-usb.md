# Clé USB

## Structure

```text
/config/buttons.csv
/config/settings.ini
/config/secrets.ini
/templates/A1.tpl
/templates/A2.tpl
/logs/YYYY-MM-DD.csv
/logs/unsynced.csv
/queue/ntfy-pending.jsonl
/captures/
```

Le firmware crée les dossiers manquants au démarrage, mais il est plus simple de
copier la structure du dossier `usb/` du dépôt.

## `/config/buttons.csv`

```csv
slot,word,ntfy_label
A1,manger,Manger
A2,eau,Eau
A3,jouer,Jouer
A4,dehors,Dehors
B1,dedans,Dedans
B2,calin,Câlin
```

- `slot` : position physique sur les tiles.
- `word` : identifiant court dans les logs.
- `ntfy_label` : texte humain dans les notifications.

## `/config/secrets.ini`

Créer ce fichier à partir de `secrets.example.ini`.

```ini
wifi_ssid=NomDuHotspot
wifi_pass=MotDePasseHotspot
ntfy_host=ntfy.sh
ntfy_topic=topic-prive-long
ntfy_token=
ntfy_tls=true
timezone_offset_minutes=120
```

Utiliser un topic `ntfy` long et difficile à deviner. Ne pas versionner
`secrets.ini`.

## `/config/settings.ini`

```ini
confidence_threshold=0.75
match_distance_at_zero=0.12
min_trigger_abs=650
trigger_multiplier=4.0
capture_ms=1500
cooldown_ms=800
calibration_samples=12
```

Ces valeurs sont rechargeables avec la commande série :

```text
reload
```

## Templates

Les fichiers `/templates/*.tpl` sont générés par le firmware. Exemple :

```text
TPB_TEMPLATE_V1
slot=A1
word=manger
samples=12
features=0.0612,0.0588,...
```

Ils sont propres à tes boutons, à tes enregistrements et au placement micro.

## Logs

```csv
timestamp,slot,word,confidence,duration_ms,noise_floor,notified
2026-06-16T14:32:08+02:00,A1,manger,0.86,1500,142,true
```

Si l'heure NTP n'est pas encore synchronisée, les lignes vont dans :

```text
/logs/unsynced.csv
```

## File de notifications

Si le Wi-Fi échoue :

```text
/queue/ntfy-pending.jsonl
```

Le firmware tente de vider cette file toutes les 30 secondes quand le Wi-Fi est
disponible.
