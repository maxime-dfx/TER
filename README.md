# Projet TER : Calcul Numérique et Traitement d'Image

Ce projet combine un solveur éléments finis et un module de segmentation d'image utilisant OpenCV.

## 📂 Architecture du projet

Le projet est structuré de manière modulaire :

```text
projet/
├── bin/                    # Exécutables finaux (générés ici)
├── data/    
│   ├── geo/                # Géométrie à mettre dans GMSH
│   ├── images_raw/         # Images d'entrée pour la segmentation
│   ├── images_segmenté/    # Images segmenté
│   ├── maillage/           # Maillages (.mesh) pour le solveur
├── finite_elements/        # Module de résolution numérique (Solver)
│   ├── src/                # Fichiers sources (.cpp)
│   └── include/            # En-têtes (.h)
├── traitement_image/       # Module de traitement d'image (OpenCV)
│   ├── src/
│   └── include/
├── libs/                   # Bibliothèques externes locales
│   ├── Eigen/              # Algèbre linéaire
│   └── opencv_install/     # OpenCV (compilé localement)
└── Makefile                # "Master" Makefile pour tout piloter