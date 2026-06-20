# Audio

L'ancien projet utilisait des fichiers WAV joues par l'Arduino. La V1 actuelle
fait l'inverse: les sons viennent des boutons enregistrables, et le GIGA les
ecoute.

Les donnees audio utiles sont maintenant:

- les templates appris dans `/templates/*.tpl` sur la cle USB;
- les logs de reconnaissance dans `/logs/*.csv`;
- la file de notifications dans `/queue/ntfy-pending.jsonl`.

Il n'y a donc plus de fichiers WAV a copier pour le firmware actuel.
