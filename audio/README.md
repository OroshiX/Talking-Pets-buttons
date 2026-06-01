# Fichiers audio

Le sketch cherche les fichiers suivants a la racine de la cle USB:

| Mot        | Fichier       |
|------------|---------------|
| manger     | `MANGER.WAV`  |
| eau        | `EAU.WAV`     |
| jouer      | `JOUER.WAV`   |
| dehors     | `DEHORS.WAV`  |
| dedans     | `DEDANS.WAV`  |
| calin      | `CALIN.WAV`   |
| brosser    | `BROSSER.WAV` |
| dormir     | `DORMIR.WAV`  |
| oui        | `OUI.WAV`     |
| non        | `NON.WAV`     |
| aide       | `AIDE.WAV`    |
| encore     | `ENCORE.WAV`  |
| fini       | `FINI.WAV`    |
| litiere    | `LITIERE.WAV` |
| friandise  | `FRIAND.WAV`  |
| ouvrir     | `OUVRIR.WAV`  |
| voir       | `VOIR.WAV`    |
| venir      | `VENIR.WAV`   |
| mal        | `MAL.WAV`     |
| content    | `CONTENT.WAV` |
| peur       | `PEUR.WAV`    |
| toi        | `TOI.WAV`     |
| moi        | `MOI.WAV`     |
| maintenant | `MAINT.WAV`   |

Format recommande:

- WAV;
- PCM;
- mono;
- 16-bit;
- 16000 Hz pour des voix courtes.

Conversion avec `ffmpeg`:

```sh
ffmpeg -i source.wav -ac 1 -ar 16000 -sample_fmt s16 MANGER.WAV
```

Garder les noms sans accents et sans espaces.
