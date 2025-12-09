# -----------------------------------------
# SCRIPT GNUPLOT FINAL
# -----------------------------------------

# 1. Configuration de l'image
set terminal pngcairo size 1200,800 enhanced font 'Verdana,10'
set output 'data/resultats/graphique/graphiquesimulation_result.png'

# 2. Configuration du graphique
set title "Résultats FEM : Déformée"
set xlabel "X (mm)"
set ylabel "Y (mm)"
set cblabel "Déplacement"

# Force les proportions (carré = carré) et ajoute la grille
set size ratio -1
set grid

# 3. IMPORTANT : Format des nombres
# Force Gnuplot à comprendre que le point "." est le séparateur décimal
# (nécessaire si votre PC est en français et attend des virgules)
set decimalsign '.'

# Palette de couleurs
set palette rgbformulae 33,13,10

# 4. Tracé
# Pas de 'every ::1' car pas d'en-tête
# 'ps 1.5' augmente la taille des points pour qu'ils soient bien visibles
plot "data/resultats/data/resultats.dat" \
     using 1:2:3 \
     with points pt 7 ps 1.5 palette notitle

print "Graphique généré."