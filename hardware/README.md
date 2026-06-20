# Support 3D pour bouton

`button-riser.stl` est le fichier pret à ouvrir dans PrusaSlicer. Les unites
sont en millimetres.

`button-riser.scad` est la source paramétrique OpenSCAD si tu veux changer le
diamètre du bouton ou la hauteur du support.

`generate_button_riser_stl.py` génère le STL sans dependence externe:

```sh
python3 hardware/generate_button_riser_stl.py
```

Orientation:

- la petite fleche en relief pointe vers le micro ;
- le grand event lateral doit aussi être orienté vers le micro ;
- imprimer à plat, sans rotation.

Réglages de depart PrusaSlicer :

- PLA ou PETG ;
- hauteur de couche 0,20 mm ;
- 3 périmètres;
- 15 à 20% d'infill ;
- supports désactivés pour un premier essai.
