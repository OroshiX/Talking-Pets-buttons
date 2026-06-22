# Liste d'achat France / UE

Liste vérifiée pour un achat depuis la France le 2026-06-20. Les liens sont des
exemples de composants compatibles, avec prix affichés en euros quand la page le
permet. Il n'est pas obligatoire d'acheter exactement ces références si les
caractéristiques restent les mêmes.

Avant de commander, vérifier :

- prix TTC en euros ;
- disponibilité réelle ;
- livraison vers la France métropolitaine ;
- frais de port ;
- délai annoncé.

## Électronique principale

- [ ] Arduino GIGA R1 WiFi, recommandé depuis la boutique Arduino officielle :
  https://store.arduino.cc/products/giga-r1-wifi
  - prix constaté : 77,90 € TTC ;
  - livraison France indiquée sur la boutique Arduino ;
  - la page affiche aussi la livraison gratuite vers la France au-dessus de
    50 €.
- [ ] Module micro analogique à gain réglable MAX4466, compatible 3.3 V :
  https://www.lextronic.fr/module-microphone-amplifie-max4466-80057.html
  - prix constaté : 8,95 € TTC ;
  - disponibilité constatée : disponible ;
  - frais de port constatés : à partir de 7,90 € ;
  - référence Lextronic : `ADA1063`.
- [ ] Alternative micro si Lextronic est en rupture :
  https://www.adafruit.com/product/1063
  - ne pas l'utiliser comme premier choix depuis la France, car le prix est en
    dollars ;
  - préférer un distributeur France / UE listé par Adafruit, par exemple
    Lextronic, GoTronic ou Letmeknow.
- [ ] Clé USB-A courte, FAT32, 8 à 32 Go :
  - ordre de prix France : 5 à 12 € ;
  - acheter chez Boulanger, LDLC, Amazon.fr, Fnac/Darty ou en grande surface ;
  - choisir un modèle court pour éviter de forcer sur le port USB-A du GIGA.
- [ ] Batterie externe USB-C 5 V, ou chargeur USB-C stable :
  - ordre de prix France : 20 à 40 € pour une batterie 5 000 à 10 000 mAh ;
  - Boulanger affiche de nombreuses batteries externes USB-C en euros ;
  - une sortie 5 V / 2 A suffit largement pour la V1.
- [ ] Câble USB-C court pour alimenter le GIGA :
  - ordre de prix France : 5 à 15 € ;
  - prendre un câble charge + données si possible, même si l'alimentation seule
    suffit en usage quotidien.

## Installation physique

- [ ] 6 à 8 boutons enregistrables indépendants :
  - ordre de prix France : souvent 20 à 45 € le lot de 4, selon le diamètre et
    la qualité audio ;
  - chercher `boutons enregistrables chien chat` ou `boutons enregistrables
    animaux` sur Amazon.fr, Cdiscount, Etsy ou boutiques d'éducation animale ;
  - vérifier que chaque bouton est autonome, avec pile et message enregistrable.
- [ ] 2 tiles de yoga / tapis mousse emboîtables :
  - ordre de prix France : 15 à 35 € selon surface et épaisseur ;
  - chercher `dalles mousse emboîtables`, `tapis puzzle mousse` ou `tapis yoga
    puzzle` ;
  - préférer une mousse stable et pas trop molle, pour ne pas absorber trop de
    son.
- [ ] Fil Dupont femelle-femelle ou câble 3 conducteurs pour le micro :
  - ordre de prix France / UE : 3 à 8 € ;
  - disponible chez GoTronic, Lextronic, Amazon.fr ou kits Arduino génériques.
- [ ] Petite boîte pour le GIGA :
  - ordre de prix France : 5 à 15 € ;
  - une boîte plastique basse suffit, avec ouvertures pour USB-C, USB-A et fils
    du micro.
- [ ] Petit mât ou support orientable pour le micro :
  - ordre de prix France : 5 à 20 € ;
  - une tige imprimée en 3D, un mini trépied, ou un col de cygne léger convient.
- [ ] Velcro adhésif, patins caoutchouc, double-face mousse fine :
  - ordre de prix France : 5 à 15 € au total ;
  - disponible en magasin de bricolage, supermarché ou Amazon.fr.
- [ ] Filament PLA/PETG pour les supports 3D :
  - ordre de prix France / UE : 18 à 30 € la bobine de 1 kg ;
  - PLA recommandé pour un premier essai, PETG si les supports doivent mieux
    résister à la chaleur ou aux manipulations.

## Option bouton service

- [ ] 1 bouton poussoir normalement ouvert :
  - ordre de prix France / UE : 1 à 5 € ;
  - disponible chez GoTronic, Lextronic ou dans un kit de composants Arduino.
- [ ] 2 fils vers `D22` et `GND`.

## Application Android

- Application `ntfy`.
- Hotspot Android activé pendant les sessions d'usage.
- Topic privé long et non devinable dans `secrets.ini`.

## Fournisseurs conseillés

- Arduino Store :
  https://store.arduino.cc/products/giga-r1-wifi
  - meilleur choix pour le GIGA si le prix officiel reste correct.
- Lextronic :
  https://www.lextronic.fr/module-microphone-amplifie-max4466-80057.html
  - bon choix pour le MAX4466 en France, prix TTC en euros.
- GoTronic :
  https://www.gotronic.fr/
  - bon fournisseur français pour câbles, fils Dupont, boîtiers, connectique,
    modules et filament.
- Boulanger :
  https://www.boulanger.com/resultats?tr=batterie+externe+usb+c
  - utile pour batterie externe USB-C, chargeur et câble USB-C.
- Amazon.fr / Cdiscount / Fnac-Darty :
  - utiles pour boutons enregistrables, clé USB courte, Velcro, patins et tapis
    mousse ;
  - vérifier le vendeur réel et éviter les fiches sans dimensions claires.

## À éviter

- Micro USB : le firmware V1 attend un signal analogique sur `A0`.
- Micro électret nu sans module d'amplification : le signal sera trop faible.
- Alimentation du micro en 5 V si sa sortie peut dépasser 3.3 V.
- Page produit en dollars hors UE quand un équivalent France / UE existe.
