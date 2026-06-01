# Mise en route

## 1. Preparer Arduino IDE

Installer:

- Arduino IDE 2.x;
- package de carte `Arduino Mbed OS GIGA Boards`;
- bibliothèque `Arduino_AdvancedAnalog`;
- bibliothèque `Arduino_USBHostMbed5`.

Dans l'IDE, sélectionner:

- carte: `Arduino GIGA R1 WiFi`;
- port: le port série du GIGA.

## 2. Preparer la cle USB

- Formater la cle en FAT32.
- Utiliser un schema de partition MBR si l'outil de formatage le propose.
- Copier les fichiers WAV a la racine de la cle.
- Utiliser les noms exacts attendus par le sketch, par exemple `MANGER.WAV`.

Le sketch monte la cle sous `/usb`.

## 3. Preparer les fichiers WAV

Format attendu:

- WAV;
- PCM;
- mono;
- 16-bit;
- 16000 Hz, 22050 Hz ou 44100 Hz.

Avec `ffmpeg`, exemple:

```sh
ffmpeg -i source.wav -ac 1 -ar 16000 -sample_fmt s16 MANGER.WAV
```

## 4. Tester un bouton

1. Brancher un interrupteur entre `D2` et `GND`.
2. Brancher la cle USB dans le port USB-A du GIGA.
3. Brancher l'enceinte amplifiee au jack 3,5 mm du GIGA.
4. Téléverser `talking_pet_buttons/talking_pet_buttons.ino`.
5. Ouvrir le moniteur série a `115200` bauds.
6. Appuyer sur le bouton: le moniteur doit afficher `Pressed: manger`.

## 5. Étendre progressivement

Ordre recommande:

1. 1 bouton sur table.
2. 1 bouton dans un vrai boitier.
3. 6 boutons avec les premiers mots.
4. 24 boutons apres validation mécanique.

Ne pas fabriquer 24 boitiers avant d'avoir valide force, stabilite, bruit du
microrupteur, longueur des cables et volume audio.
