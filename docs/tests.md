# Plan de tests

## 1. Test micro

Commande:

```text
status
```

Verifier:

- `USB=mounted`;
- `noise` stable au repos;
- `threshold` superieur au bruit de fond;
- un bouton presse declenche `Audio event started`.

Si rien ne declenche:

- augmenter le gain du micro;
- rapprocher le micro;
- baisser `min_trigger_abs` dans `/config/settings.ini`.

Si tout declenche:

- baisser le gain du micro;
- augmenter `trigger_multiplier`;
- eloigner le micro de la zone de passage.

## 2. Test acoustique du support

Comparer chaque bouton dans deux configurations:

1. pose directement sur la tile;
2. pose dans le support imprime.

Garder le support si:

- le son est plus clair a l'oreille;
- la confiance moyenne augmente;
- les faux positifs ne montent pas.

## 3. Calibration

Pour chaque bouton:

```text
cal A1 12
```

Regles:

- presser depuis la vraie position finale;
- laisser le son finir entre deux pressions;
- refaire la calibration si le message du bouton est reenregistre;
- refaire la calibration si le micro change fortement de place.

## 4. Precision

Apres calibration:

- presser chaque bouton 20 fois;
- viser au moins 18 reconnaissances correctes sur 20 par bouton;
- noter les boutons confondus.

Si deux boutons sont confondus:

- enregistrer des mots plus differents;
- augmenter l'espace entre ces boutons;
- refaire les templates;
- baisser `match_distance_at_zero` ou augmenter `confidence_threshold`.

## 5. Faux positifs

Tester:

- voix humaine proche;
- television ou musique;
- pas autour des tiles;
- objet pose sur la tile;
- bouton touche sans declencher son message.

Attendu: pas de notification. Les evenements ambigus peuvent apparaitre dans
`/logs/unsynced.csv` ou le log du jour avec `slot=unknown`.

## 6. Portabilite

Sequence:

1. eteindre le GIGA;
2. deplacer tiles, boutons, micro et boitier;
3. rallumer sur batterie externe;
4. activer le hotspot Android;
5. verifier `status`;
6. presser chaque bouton.

Attendu:

- le GIGA retrouve la cle USB;
- l'heure NTP se synchronise;
- les logs vont dans `/logs/YYYY-MM-DD.csv`;
- les notifications arrivent sur Android.

## 7. Offline

Sequence:

1. couper le hotspot;
2. presser deux boutons connus;
3. verifier que `/queue/ntfy-pending.jsonl` grossit;
4. rallumer le hotspot;
5. attendre 30 secondes.

Attendu: les notifications en attente sont envoyees, puis la file est vide.
