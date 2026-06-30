# Plan de tests

## 0. Préparer le banc de test

Les commandes comme `status`, `list`, `cal A1 12` ou `testntfy` ne sont pas des
commandes à lancer dans le terminal de l'ordinateur. Ce sont des commandes série
à envoyer au firmware qui tourne sur l'Arduino GIGA.

Pour les lancer :

1. brancher le GIGA à l'ordinateur en USB-C ;
2. brancher la clé USB FAT32 sur le port USB-A du GIGA ;
3. brancher le micro MAX4466 au GIGA : `VCC` vers `3V3`, `OUT` vers `A0`,
   `GND` vers `GND` ;
4. brancher le panneau de calibration si tu veux tester la calibration sans
   ordinateur ;
5. placer les boutons à leur position réelle sur les tiles ;
6. activer le hotspot Android si le test concerne le Wi-Fi, l'heure ou `ntfy` ;
7. ouvrir le moniteur série PlatformIO, Arduino IDE, CLion ou VS Code à
   `115200` bauds ;
8. taper la commande dans le champ d'envoi du moniteur série, puis appuyer sur
   Entrée.

Si le GIGA est seulement branché à une batterie externe, il fonctionne, mais tu
ne peux pas taper `status` sans liaison série avec l'ordinateur. Pour les tests,
le plus simple est donc d'alimenter le GIGA depuis l'ordinateur en USB-C, puis de
passer sur batterie externe seulement pour le test de portabilité.

`115200` bauds est la vitesse de communication de cette console série. Dans
PlatformIO, `monitor_speed = 115200` dans `platformio.ini` règle cette vitesse
par défaut ; en terminal, `pio device monitor -b 115200` ouvre le moniteur à la
bonne vitesse.

La commande `status` affiche une ligne de diagnostic du firmware, par exemple :

```text
USB=mounted WiFi=connected time=synced noise=142 threshold=650 selected=A1 selected_no=1 calibration=active progress=3/12
```

Signification :

- `USB=mounted` : la clé USB est détectée et montée ;
- `WiFi=connected` : le GIGA est connecté au hotspot Android ;
- `time=synced` : l'heure a été récupérée par NTP ;
- `noise=142` : niveau de bruit de fond mesuré par le micro ;
- `threshold=650` : seuil au-dessus duquel un son déclenche une capture ;
- `selected=A1 selected_no=1` : bouton animal actuellement sélectionné ;
- `calibration=active progress=3/12` : calibration en cours, 3 exemples captés
  sur 12.

## 1. Test micro

Depuis le moniteur série, envoyer :

```text
status
```

Vérifier :

- `USB=mounted` ;
- `noise` stable au repos ;
- `threshold` supérieur au bruit de fond ;
- un bouton pressé déclenche `Audio event started`.

Si rien ne déclenche :

- augmenter le gain du micro ;
- rapprocher le micro ;
- baisser `min_trigger_abs` dans `/config/settings.ini`.

Si tout déclenche :

- baisser le gain du micro ;
- augmenter `trigger_multiplier` ;
- éloigner le micro de la zone de passage.

## 2. Test acoustique du support

Comparer chaque bouton dans deux configurations :

1. posé directement sur la tile ;
2. posé dans le support imprimé.

Garder le support si :

- le son est plus clair à l'oreille ;
- la confiance moyenne augmente ;
- les faux positifs ne montent pas.

## 3. Calibration

Pour chaque bouton :

```text
cal A1 12
```

Règles :

- presser depuis la vraie position finale ;
- laisser le son finir entre deux pressions ;
- refaire la calibration si le message du bouton est réenregistré ;
- refaire la calibration si le micro change fortement de place.

Test du panneau physique :

- utiliser `+` et `-` pour parcourir `0..N` sans boucle automatique ;
- vérifier que `0` éteint les 6 LEDs binaires ;
- vérifier que `000001` correspond au bouton n°1 de `buttons.csv` ;
- faire un appui long sur `CAL` à `0` : aucune calibration ne démarre ;
- sélectionner le bouton n°1, puis faire un appui long sur `CAL` ;
- vérifier que la LED intégrée clignote 3 fois puis reste allumée ;
- pendant la calibration, appuyer sur `+` et `-` : la sélection ne doit pas
  changer ;
- après 12 exemples, vérifier que la sélection passe au bouton suivant.

## 4. Précision

Après calibration :

- presser chaque bouton 20 fois ;
- viser au moins 18 reconnaissances correctes sur 20 par bouton ;
- noter les boutons confondus.

Si deux boutons sont confondus :

- enregistrer des mots plus différents ;
- augmenter l'espace entre ces boutons ;
- refaire les templates ;
- baisser `match_distance_at_zero` ou augmenter `confidence_threshold`.

## 5. Faux positifs

Tester :

- voix humaine proche ;
- télévision ou musique ;
- pas autour des tiles ;
- objet posé sur la tile ;
- bouton touché sans déclencher son message.

Attendu : pas de notification. Les événements ambigus peuvent apparaître dans
`/logs/unsynced.csv` ou le log du jour avec `slot=unknown`.

## 6. Portabilité

Séquence :

1. éteindre le GIGA ;
2. déplacer tiles, boutons, micro et boîtier ;
3. rallumer sur batterie externe ;
4. activer le hotspot Android ;
5. si un ordinateur est branché temporairement en USB-C, vérifier `status` ;
6. presser chaque bouton.

Attendu :

- le GIGA retrouve la clé USB ;
- l'heure NTP se synchronise ;
- les logs vont dans `/logs/YYYY-MM-DD.csv` ;
- les notifications arrivent sur Android.

Sans ordinateur branché, ce test se valide surtout par les notifications Android
et par les fichiers de log créés sur la clé USB après extinction propre du GIGA.

## 7. Offline

Séquence :

1. couper le hotspot ;
2. presser deux boutons connus ;
3. vérifier que `/queue/ntfy-pending.jsonl` grossit ;
4. rallumer le hotspot ;
5. attendre 30 secondes.

Attendu : les notifications en attente sont envoyées, puis la file est vide.
