# --- MASTER MAKEFILE (CHEF D'ORCHESTRE) ---

# Définition des dossiers
DIR_FE = finite_elements
DIR_IMG = traitement_image
GNUPLOT_DIR = data/resultats/gnuplot

# Les cibles qui ne sont pas des fichiers
.PHONY: all clean fe img help

# 1. Commande par défaut (quand on tape juste "make")
all: fe img
	@echo "✅ Tout est compilé ! Les exécutables sont dans le dossier 'bin'."

# 2. Compiler seulement la partie Elements Finis
fe:
	@echo "-----------------------------------"
	@echo "🔧 Compilation du module Elements Finis..."
	@echo "-----------------------------------"
	@$(MAKE) -C $(DIR_FE)

# 3. Compiler seulement la partie Traitement Image
img:
	@echo "-----------------------------------"
	@echo "📸 Compilation du module Traitement Image..."
	@echo "-----------------------------------"
	@$(MAKE) -C $(DIR_IMG)

# 4. Tout nettoyer (supprimer les .o et les .exe)
clean:
	@echo "🧹 Nettoyage en cours..."
	@$(MAKE) -C $(DIR_FE) clean
	@$(MAKE) -C $(DIR_IMG) clean
	rm -rf bin/*
	@echo "✨ Tout est propre."

# 5. Aide
help:
	@echo "Commandes disponibles :"
	@echo "  make        : Compile tout le projet"
	@echo "  make fe     : Compile uniquement le solver éléments finis"
	@echo "  make img    : Compile uniquement le traitement d'image"
	@echo "  make clean  : Supprime tous les fichiers temporaires et exécutables"

plot:
	@echo "Génération des graphiques..."
	gnuplot $(GNUPLOT_DIR)/Heatmap.gp
	gnuplot $(GNUPLOT_DIR)/Poisson.gp
	gnuplot $(GNUPLOT_DIR)/Young.gp
	@echo "Graphiques générés dans data/resultats/graphique !"