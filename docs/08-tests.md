# Plan de tests

## 1. Test micro

Commande :

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
5. vérifier `status` ;
6. presser chaque bouton.

Attendu :

- le GIGA retrouve la clé USB ;
- l'heure NTP se synchronise ;
- les logs vont dans `/logs/YYYY-MM-DD.csv` ;
- les notifications arrivent sur Android.

## 7. Offline

Séquence :

1. couper le hotspot ;
2. presser deux boutons connus ;
3. vérifier que `/queue/ntfy-pending.jsonl` grossit ;
4. rallumer le hotspot ;
5. attendre 30 secondes.

Attendu : les notifications en attente sont envoyées, puis la file est vide.
