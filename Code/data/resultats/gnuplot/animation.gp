# ============================================================================
# SCRIPT GNUPLOT - ANIMATION FEM COMPOSITE (VERSION PROFESSIONNELLE)
# ============================================================================

reset
set encoding utf8

# ==========================================
# 1. CONFIGURATION VISUELLE AVANCÉE
# ==========================================

# Titre principal avec style
set title "Simulation FEM - Déformation du Composite" \
    font "Arial Bold,16" \
    textcolor rgb "#2c3e50" \
    offset 0,0.5

# Labels des axes
set xlabel "Position X (mm)" font "Arial,13" textcolor rgb "#34495e" offset 0,-0.5
set ylabel "Position Y (mm)" font "Arial,13" textcolor rgb "#34495e" offset -1,0

# Grille professionnelle (double niveau)
set grid xtics ytics mxtics mytics \
    linetype -1 linewidth 0.8 linecolor rgb "#bdc3c7", \
    linetype -1 linewidth 0.3 linecolor rgb "#ecf0f1"
set mxtics 5
set mytics 5

# Bordures élégantes
set border linewidth 1.5 linecolor rgb "#7f8c8d"
set tics nomirror font "Arial,11"

# ==========================================
# 2. ASPECT RATIO ET DIMENSIONS
# ==========================================

# Force l'échelle 1:1 (crucial pour la mécanique)
set size ratio -1

# Ajustement dynamique des limites (modifie selon ton maillage)
set xrange [-1:11]
set yrange [-2:2]

# Marges optimisées
set lmargin at screen 0.12
set rmargin at screen 0.90
set bmargin at screen 0.15
set tmargin at screen 0.90

# ==========================================
# 3. STYLES DE TRACÉ AVANCÉS
# ==========================================

# Style 1: Maillage déformé (bleu professionnel)
set style line 1 lc rgb "#3498db" lw 1.8 dt solid

# Style 2: Maillage initial en transparence (optionnel)
set style line 2 lc rgb "#95a5a6" lw 0.8 dt (5,3)

# Style 3: Contours remplis (mode alternatif)
set style line 3 lc rgb "#e74c3c" lw 1.2 dt solid

# Style 4: Points de Gauss (mode debug)
set style line 4 lc rgb "#f39c12" pt 7 ps 0.5

# ==========================================
# 4. CONFIGURATION DE SORTIE
# ==========================================

# --- MODE 1: AFFICHAGE INTERACTIF (décommenter pour test) ---
# set term qt size 1200,800 font "Arial,11" persist enhanced
# pause_time = 0.05

# --- MODE 2: GIF HAUTE QUALITÉ (actif par défaut) ---
set term gif animate delay 8 size 1200,800 optimize font "Arial,11" enhanced
set output "data/resultats/graphique/animation_FEM_HD.gif"
pause_time = 0

# --- MODE 3: GIF ULTRA RAPIDE (pour tests) ---
# set term gif animate delay 3 size 800,600 optimize
# set output "data/resultats/graphique/animation_FAST.gif"
# pause_time = 0

# --- MODE 4: EXPORT PNG SÉQUENTIEL (pour vidéo) ---
# set term pngcairo size 1920,1080 enhanced font "Arial,14"
# (voir boucle modifiée en bas)

# ==========================================
# 5. LÉGENDE ET ANNOTATIONS
# ==========================================

# Position de la légende
set key top right \
    box linewidth 1 \
    font "Arial,10" \
    textcolor rgb "#2c3e50" \
    spacing 1.2 \
    enhanced

# Timestamp (optionnel - décommenter si besoin)
# set timestamp "Généré le %d/%m/%Y" font "Arial,8"

# ==========================================
# 6. VARIABLES DE CONTRÔLE
# ==========================================

limit = 50                  # Nombre de frames
nb_triangles = 17216        # Pour info dans le titre
deformation_max = 0.01      # Déformation appliquée (1%)

# ==========================================
# 7. BOUCLE D'ANIMATION PRINCIPALE
# ==========================================

print "╔═══════════════════════════════════════════════╗"
print "║   GÉNÉRATION ANIMATION FEM EN COURS...        ║"
print "╠═══════════════════════════════════════════════╣"
print sprintf("║   Frames      : %d                           ║", limit+1)
print sprintf("║   Triangles   : %d                         ║", nb_triangles)
print sprintf("║   Déformation : %.1f%%                        ║", deformation_max*100)
print "╚═══════════════════════════════════════════════╝"

do for [i=0:limit] {
    
    # Nom du fichier de données
    file_name = sprintf("data/resultats/data/anim_%d.txt", i)
    
    # Calcul du pourcentage de progression
    progress = 100.0 * i / limit
    
    # Titre dynamique avec informations
    set title sprintf("Simulation FEM : Frame %d/%d (%.1f%% de déformation)", \
                      i, limit, progress) \
        font "Arial Bold,16" \
        textcolor rgb "#2c3e50"
    
    # Tracé principal
    plot file_name \
        using 1:2 \
        with lines \
        linestyle 1 \
        title sprintf("Maillage déformé (t=%.2f)", progress/100.0)
    
    # Affichage de progression dans le terminal
    if (i % 10 == 0 || i == limit) {
        print sprintf("  → Frame %3d/%d générée [%3.0f%%]", i, limit, progress)
    }
    
    # Pause pour mode interactif
    if (pause_time > 0) { pause pause_time }
}

print "╔═══════════════════════════════════════════════╗"
print "║   ✓ ANIMATION TERMINÉE AVEC SUCCÈS            ║"
print "╚═══════════════════════════════════════════════╝"

# ==========================================
# 8. BONUS: MODE COMPARAISON (décommenter)
# ==========================================

# Affiche maillage initial + déformé en superposition
# set term pngcairo size 1200,800 enhanced font "Arial,11"
# set output "data/resultats/graphique/comparaison_avant_apres.png"
# 
# set title "Comparaison : Maillage Initial vs. Déformé"
# 
# plot "data/resultats/data/anim_0.txt" \
#      using 1:2 with lines ls 2 title "Initial", \
#      "data/resultats/data/anim_50.txt" \
#      using 1:2 with lines ls 1 title "Déformé (100%)"
# 
# print "  → Image de comparaison créée"

# ==========================================
# 9. BONUS: EXPORT PNG SÉQUENTIEL
# ==========================================

# Décommenter pour créer des PNG individuels (utile pour montage vidéo)
# set term pngcairo size 1920,1080 enhanced font "Arial,14"
# 
# do for [i=0:limit] {
#     set output sprintf("data/resultats/frames/frame_%04d.png", i)
#     file_name = sprintf("data/resultats/data/anim_%d.txt", i)
#     set title sprintf("Frame %d/%d", i, limit)
#     plot file_name using 1:2 with lines ls 1 notitle
# }
# 
# print "  → Frames PNG exportées (convertir avec ffmpeg)"
# print "    Commande: ffmpeg -framerate 20 -i frame_%04d.png -c:v libx264 animation.mp4"