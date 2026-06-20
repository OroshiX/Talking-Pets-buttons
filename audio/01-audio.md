# Audio

L'ancien projet utilisait des fichiers WAV joués par l'Arduino. La V1 actuelle
fait l'inverse : les sons viennent des boutons enregistrables, et le GIGA les
écoute.

Les données audio utiles sont maintenant :

- les templates appris dans `/templates/*.tpl` sur la clé USB ;
- les logs de reconnaissance dans `/logs/*.csv` ;
- la file de notifications dans `/queue/ntfy-pending.jsonl`.

Il n'y a donc plus de fichiers WAV à copier pour le firmware actuel.
