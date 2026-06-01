# Tests

## Test électrique minimum

- Multimètre en mode continuité.
- Bouton relache: pas de continuité entre les deux fils.
- Bouton appuye: continuité entre les deux fils.
- Aucun fil du bouton ne doit etre relie au 5 V.

## Test Arduino sans audio

Brancher un bouton entre `D2` et `GND`, ouvrir le moniteur série, puis vérifier :

- un appui affiche `Pressed: manger`;
- un appui long ne spamme pas le moniteur;
- un relâchement puis nouvel appui retrigger correctement;
- un cable remue ne crée pas de faux appuis.

## Test audio

- `MANGER.WAV` est present a la racine de la cle USB.
- La cle est FAT32.
- L'enceinte est allumée, amplifiée, et branchée en AUX.
- Un appui sur `D2` joue le son.

## Test 6 boutons

- Chaque pin de `D2` a `D7` joue le bon fichier.
- Deux appuis rapides sont joues dans l'ordre quand la file n'est pas pleine.
- Si un fichier manque, le moniteur serie affiche l'erreur mais le systeme garde
  les autres boutons utilisables.

## Test mécanique avec l'animal

- Le bouton ne glisse pas quand il est pousse lateralement.
- La plaque d'appui est assez grande pour une patte.
- L'appui se déclenche sans que le chat doive mettre tout son poids dessus.
- Les fils ne sont pas attrayants ni accessibles a machouiller.

## Si faux appuis

Essayer dans cet ordre:

1. Raccourcir ou torsader les deux fils du bouton.
2. Eloigner les cables des alimentations et moteurs.
3. Ajouter un condensateur 100 nF entre l'entrée du bouton et `GND`.
4. Augmenter `kDebounceMs` dans le sketch, par exemple de `35` a `60`.
