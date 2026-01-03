set terminal pngcairo size 1000,600 enhanced font 'Verdana,10'
# Chemin relatif depuis la racine 'Code'
set output 'data/resultats/graphique/champ_deplacement.png'

set title "Carte du Déplacement (Magnitude)"
set xlabel "X (mm)"
set ylabel "Y (mm)"

# --- CORRECTION APPLIQUÉE ---
# 1. Active l'interpolation de grille (100x100 points, lissage qnorm 2)
# Cela permet de transformer le nuage de points non-structuré en surface
set dgrid3d 100,100

# 2. Vue de dessus (Heatmap 2D)
set view map
set pm3d map
# -----------------------------

set palette rgbformulae 33,13,10
set size ratio -1

# Colonne 1:X, 2:Y, 5:Magnitude (Calculé dans le C++: sqrt(ux^2+uy^2))
splot 'data/resultats/data/champ_complet.dat' using 1:2:5 with pm3d notitle