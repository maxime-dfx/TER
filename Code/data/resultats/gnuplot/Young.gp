# ==========================================
# CONFIGURATION DE SORTIE
# ==========================================
set terminal pngcairo size 1200,800 enhanced font 'Verdana,12'
# Vérifie que le dossier existe ou ajuste le chemin selon tes besoins
set output 'data/resultats/graphique/resultat_Young.png'

# ==========================================
# STYLE & FORMATAGE
# ==========================================
set title "Essai de Traction Virtuel (FEM)" font ",16"
set xlabel "Déformation / Strain (%)" font ",14"
set ylabel "Contrainte / Stress (MPa)" font ",14"

set grid lc rgb "#DDDDDD" lt 1  # Grille légère
set key top left box opaque     # Légende avec fond blanc

# ==========================================
# REGRESSION LINEAIRE (Calcul du Module E)
# ==========================================
# Loi de Hooke : Sigma = E * Epsilon (y = ax)
f(x) = E_eff * x

# Ajustement (fit) aux données
# On utilise le fichier généré par le C++ (séparateur espace par défaut)
fit f(x) 'data/resultats/data/courbe_traction.dat' using 1:2 via E_eff

# Préparation du label pour la légende
title_fit = sprintf("Régression Linéaire (E = %.0f MPa)", E_eff)

# ==========================================
# TRACÉ
# ==========================================
# $1*100 : On convertit le strain (0.001) en pourcentage (0.1%) pour l'axe X
# f(x/100) : Comme l'axe X est en %, il faut rediviser par 100 pour le calcul physique

plot 'data/resultats/data/courbe_traction.dat' using ($1*100):2 \
        with points pt 7 ps 1.5 lc rgb "blue" title "Points FEM", \
     f(x/100) \
        with lines lw 2 lc rgb "red" title title_fit

# Affichage du résultat dans le terminal
print "---------------------------------------------------"
print sprintf("Graphique généré : data/resultats/courbe_traction_gnuplot.png")
print sprintf("Module d'Young calculé par Gnuplot : %.2f MPa", E_eff)
print "---------------------------------------------------"