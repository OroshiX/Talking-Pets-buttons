# Liste d'achat France / UE

Les liens sont des exemples de composants compatibles. Il n'est pas obligatoire
d'acheter exactement ces references, tant que les caractéristiques restent les
memes.

## Prototype 6 boutons

- 6 x microrupteur a levier long basse force:
  https://www.gotronic.fr/art-microrupteur-ms12l-4327.htm
- 6 x boitier ou base pour bouton DIY:
  https://www.gotronic.fr/art-boitier-pour-bp-urgence-40373.htm
- Cable deux conducteurs:
  https://www.gotronic.fr/art-bobine-de-cable-50-m-fc24rbl50-29804.htm
- Shield a borniers Mega/GIGA:
  https://www.gotronic.fr/art-shield-a-borniers-mega-dfr0921-35746.htm
- Option connecteurs amovibles deux broches:
  https://www.gotronic.fr/art-rallonge-jst-xh-2-cts-36136.htm
- Condensateurs 100 nF si les cables longs déclenchent des faux appuis:
  https://www.gotronic.fr/art-condensateur-ceramique-100-nf-40054.htm
- Enceinte amplifiée AUX:
  https://fr.creative.com/p/speakers/creative-pebble-v2

## Passage a 24 boutons

- 24 a 30 microrupteurs MS12L.
- 24 a 30 boitiers/bases.
- 50 m de cable deux conducteurs minimum, plus si les boutons sont disperses.
- Un grand boitier central, par exemple:
  https://www.gotronic.fr/art-boitier-abs-etanche-g373-6618.htm
- Borniers, connecteurs ou petits dominos pour faire un rail `GND` propre.
- Patins caoutchouc, velcro ou tapis antidérapant pour stabiliser les boutons.

## Alimentation

- Arduino GIGA alimente par USB-C.
- Enceinte amplifiée alimentée selon son modèle, souvent USB.
- Éviter d'alimenter des modules externes depuis les GPIO.

## Option fallback audio

Si la lecture WAV sur le GIGA devient trop contraignante, ajouter un lecteur MP3
dedie comme DFPlayer Mini:

- Documentation officielle: https://wiki.dfrobot.com/dfr0299

Ce fallback demanderait un autre sketch, une carte microSD, et une liaison serie
entre le GIGA et le module audio.
