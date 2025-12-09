# ==========================================
# CONFIGURATION DE SORTIE
# ==========================================
set terminal pngcairo size 1200,800 enhanced font 'Verdana,12'
set output 'data/resultats/graphique/resultat_Poisson.png'

# ==========================================
# STYLE & FORMATAGE
# ==========================================
set title "Identification du Coefficient de Poisson (FEM)" font ",16"
set xlabel "Déformation Longitudinale / Axial Strain (%)" font ",14"
set ylabel "Déformation Transverse / Transverse Strain (%)" font ",14"

set grid lc rgb "#DDDDDD" lt 1  # Grille légère
set key bottom right box opaque # Légende en bas à droite

# ==========================================
# REGRESSION LINEAIRE (Calcul de Nu)
# ==========================================
# Relation : -Eps_yy = Nu * Eps_xx
g(x) = nu_eff * x

# Ajustement (fit) aux données
# Col 1 : Eps_xx (X)
# Col 3 : Eps_yy (On prend l'opposé -$3 pour avoir une valeur positive)
fit g(x) 'data/resultats/data/courbe_traction.dat' using 1:(-$3) via nu_eff

# Préparation du label pour la légende
title_fit = sprintf("Régression Linéaire (Nu = %.4f)", nu_eff)

# ==========================================
# TRACÉ
# ==========================================
# $1*100   : Conversion Eps_xx en %
# -$3*100  : Conversion -Eps_yy en %
# g(x/100)*100 : La fonction prend des valeurs brutes (x/100), 
#                le résultat est brut, on remultiplie par 100 pour l'affichage en %

plot 'data/resultats/data/courbe_traction.dat' using ($1*100):(-$3*100) \
        with points pt 7 ps 1.5 lc rgb "blue" title "Points FEM", \
     g(x/100)*100 \
        with lines lw 2 lc rgb "red" title title_fit

# Affichage du résultat dans le terminal
print "---------------------------------------------------"
print sprintf("Graphique généré : data/resultats/graphique/resultat_Poisson.png")
print sprintf("Coefficient de Poisson calculé : %.4f", nu_eff)
print "---------------------------------------------------"