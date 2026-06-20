# Support 3D pour bouton

`button-riser.stl` est le fichier prêt à ouvrir dans PrusaSlicer. Les unités
sont en millimètres.

`button-riser.scad` est la source paramétrique OpenSCAD si tu veux changer le
diamètre du bouton ou la hauteur du support. Le design est un cylindre creux :
le bouton repose sur une petite protubérance intérieure suspendue, et la chambre
centrale reste dégagée sous le haut-parleur.

`generate_button_riser_stl.py` génère le STL sans dépendance externe :

```sh
python3 hardware/generate_button_riser_stl.py
```

Orientation :

- la petite flèche en relief pointe vers le micro ;
- le grand évent latéral doit aussi être orienté vers le micro ;
- le bouton se pose par-dessus, sur la petite protubérance intérieure ;
- imprimer à plat, sans rotation.

Réglages de départ PrusaSlicer :

- PLA ou PETG ;
- hauteur de couche 0,20 mm ;
- 3 périmètres ;
- 15 à 20% d'infill ;
- supports désactivés pour un premier essai.
