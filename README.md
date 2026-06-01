# Boutons parlants DIY pour animaux

Projet Arduino GIGA R1 WiFi pour fabriquer des boutons passifs sans batterie:
chaque bouton est un simple interrupteur deux fils, et une boite centrale joue le
son correspondant sur une enceinte amplifiee.

## Contenu

- `src/main.cpp`: sketch PlatformIO/Arduino pour 24 boutons.
- `docs/cablage.md`: schema de cablage et logique electrique.
- `docs/liste-achats.md`: composants recommandes avec liens France/UE.
- `docs/mise-en-route.md`: configuration PlatformIO, cle USB et premiers tests.
- `docs/tests.md`: checklist de validation avant de construire 24 boutons.
- `audio/README.md`: format et noms des fichiers WAV.

## Principe

- Un cote de chaque bouton va vers une entree numerique du GIGA.
- L'autre cote va vers `GND`.
- Le sketch active `INPUT_PULLUP`, donc un bouton appuye lit `LOW`.
- Les sons sont des fichiers WAV sur une cle USB FAT32 branchee au port USB-A.
- La sortie audio jack 3,5 mm du GIGA va vers une enceinte amplifiée AUX.

## Demarrage rapide

1. Ouvrir le projet PlatformIO dans CLion ou VS Code.
2. Verifier que `platformio.ini` utilise `board = giga_r1_m7`.
3. Laisser PlatformIO installer les bibliothèques déclarées dans `lib_deps`.
4. Formater une cle USB en FAT32 avec schema MBR.
5. Copier des fichiers WAV mono 16-bit PCM a la racine de la cle, avec les noms
   listes dans `audio/README.md`.
6. Compiler et téléverser sur l'Arduino GIGA R1 WiFi.
7. Brancher un bouton entre `D2` et `GND`, puis ouvrir le moniteur série a
   `115200` bauds pour verifier `Pressed: manger`.

## Securite

- Ne jamais envoyer de 5 V sur les entrees du GIGA; les GPIO sont en 3,3 V.
- Ne jamais brancher un haut-parleur passif directement sur le jack ou les DAC.
  Utiliser une enceinte amplifiee ou un ampli audio.
- Garder soudures, fils denudes, alimentation et cle USB dans une boite centrale
  hors de portee des animaux.

## Sources techniques

- Arduino GIGA R1 WiFi: https://docs.arduino.cc/hardware/giga-r1-wifi
- Audio GIGA officiel: https://docs.arduino.cc/tutorials/giga-r1-wifi/giga-audio
- USB GIGA officiel: https://docs.arduino.cc/tutorials/giga-r1-wifi/giga-usb
