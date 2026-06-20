# Talking Pet Buttons - ecoute audio portable

Ce projet transforme un Arduino GIGA R1 WiFi en boitier d'ecoute pour boutons
enregistrables independants. Les boutons ne sont pas modifies: le GIGA ecoute
leur son avec un micro analogique, reconnait chaque bouton par empreinte audio,
journalise les appuis sur une cle USB et envoie une notification `ntfy` via le
hotspot Android.

## Principe

```mermaid
flowchart LR
  A["Bouton enregistrable"] --> B["Son dans la piece"]
  B --> C["Micro analogique A0"]
  C --> D["Arduino GIGA"]
  D --> E["Fingerprint audio"]
  E --> F["Log CSV sur cle USB"]
  E --> G["ntfy via hotspot Android"]
```

La V1 vise 6 a 8 boutons sur 2 tiles de yoga. Chaque bouton est placé dans un
support imprimé en 3D de type cylindre creux : une petite protubérance
intérieure suspendue porte le bouton, la zone sous le haut-parleur reste
dégagée, et les évents latéraux orientent le son vers le micro.

## Contenu du projet

- `src/main.cpp`: firmware PlatformIO pour Arduino GIGA R1 WiFi.
- `usb/config/`: exemples de fichiers à copier sur la cle USB.
- `hardware/button-riser.stl`: support 3D prêt pour PrusaSlicer.
- `hardware/button-riser.scad`: source paramétrique OpenSCAD du support.
- `docs/agencement.md`: placement des tiles, du GIGA, du micro et des boutons.
- `docs/schemas-branchement.md`: branchements micro, USB, alimentation et flux.
- `docs/calibration.md`: apprentissage des boutons avec les commandes série.
- `docs/cle-usb.md`: structure de la cle USB et formats des fichiers.
- `docs/tests.md`: checklist de validation avant usage quotidien.
- `docs/images/`: schemas SVG exportables/imprimables.

## Materiel V1

- Arduino GIGA R1 WiFi.
- Module micro analogique 3.3 V a gain réglable, type MAX4466.
- Cle USB-A FAT32.
- Batterie externe USB-C ou chargeur USB-C.
- Telephone Android avec hotspot et application `ntfy`.
- 6 a 8 boutons enregistrables indépendants.
- 2 tiles de yoga emboitables.
- Supports imprimés en 3D, un par bouton.

## Démarrage rapide

1. Copier `usb/config/buttons.csv`, `usb/config/settings.ini` et un
   `secrets.ini` base sur `usb/config/secrets.example.ini` à la racine d'une
   cle USB FAT32, en gardant les dossiers.
2. Brancher le micro :
   - `VCC` vers `3V3`;
   - `GND` vers `GND`;
   - `OUT` vers `A0`.
3. Brancher la clé USB dans le port USB-A du GIGA.
4. Alimenter le GIGA en USB-C.
5. Compiler et téléverser avec PlatformIO.
6. Ouvrir le moniteur série a `115200` bauds.
7. Lancer `cal A1 12`, puis presser le bouton A1 12 fois.
8. Répéter pour chaque bouton.
9. Lancer `testntfy`, puis verifier l'application Android.

## Commandes série

```text
help              affiche les commandes
list              affiche les boutons et templates charges
status            affiche USB, Wi-Fi, heure, bruit et seuil
reload            recharge la config et les templates depuis la cle USB
cal A1 [12]       apprend les prochains appuis pour le slot A1
cancel            annule une calibration active
testntfy          envoie une notification de test
```

## Important

- Le GIGA ne fait pas de reconnaissance vocale generale. Il compare des sons de
  boutons deja appris.
- Si tu reenregistres le message d'un bouton, il faut refaire sa calibration.
- Si tu changes fortement la position du micro ou des boutons, refais au moins
  un test de precision.
- Les notifications sont mises en attente dans `/queue/ntfy-pending.jsonl` si le
  hotspot est indisponible.
