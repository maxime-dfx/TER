# ==============================================================================
#                 MASTER MAKEFILE - CHEF D'ORCHESTRE DU TER
#           Centralise la compilation, le nettoyage et l'analyse
# ==============================================================================

# --- DÉFINITION DES CHEMINS ---
DIR_BIN  = bin
DIR_CORE = core
DIR_FE   = $(DIR_CORE)/finite_elements
DIR_IMG  = $(DIR_CORE)/traitement_image

DIR_DATA = data
DIR_RAW  = $(DIR_DATA)/raw
DIR_OUT  = $(DIR_DATA)/outputs
DIR_RES  = results

GNUPLOT_DIR = scripts/gnuplot # Adapté si tu as rangé tes .gp ici

# --- COMMANDES ---
.PHONY: all fe img clean clean-data clean-legacy fclean plot help

# 1. Compilation Totale
all: fe img
	@echo "✅ Tout est compilé ! Les exécutables sont dans '$(DIR_BIN)/'."

# 2. Module Éléments Finis
fe:
	@echo "🔧 Compilation du solver FE..."
	@$(MAKE) -C $(DIR_FE)

# 3. Module Traitement d'Image
img:
	@echo "📸 Compilation du module Vision..."
	@$(MAKE) -C $(DIR_IMG)

# 4. Nettoyage Compilation (Objets .o)
clean:
	@echo "🧹 Nettoyage des fichiers objets..."
	@$(MAKE) -C $(DIR_FE) clean
	@$(MAKE) -C $(DIR_IMG) clean
	@rm -rf $(DIR_BIN)/*

# 5. Nettoyage sélectif des Données (PROTECTION DES PHOTOS)
# On efface les sorties de simulation mais on ne touche JAMAIS au dossier 'raw'
clean-data:
	@echo "🗑️  Purge des résultats (.mesh, .vtk, .toml, .csv, .png) dans $(DIR_OUT)..."
	@find $(DIR_OUT) -type f \( -name "*.mesh" -o -name "*.vtk" -o -name "*.toml" -o -name "*.csv" -o -name "*.png" \) -delete
	@echo "✅ Résultats nettoyés. Tes photos dans $(DIR_RAW) sont en sécurité."

# 6. Suppression des anciens dossiers "Vestiges"
# À utiliser une seule fois pour valider ta nouvelle structure
clean-legacy:
	@echo "🚜 Suppression des anciens dossiers inutiles..."
	@rm -rf $(DIR_DATA)/maillage $(DIR_DATA)/etude_* $(DIR_DATA)/stats_* $(DIR_DATA)/convergence $(DIR_DATA)/validation_suite
	@echo "✅ Vestiges supprimés."

# 7. Reset TOTAL (Compilation + Données)
fclean: clean clean-data
	@echo "🚀 Projet totalement réinitialisé."

# 8. Génération des Graphiques
plot:
	@echo "📊 Génération des graphiques Gnuplot..."
	@mkdir -p $(DIR_RES)
	gnuplot $(GNUPLOT_DIR)/Heatmap.gp
	gnuplot $(GNUPLOT_DIR)/Poisson.gp
	gnuplot $(GNUPLOT_DIR)/Young.gp
	@echo "✅ Graphiques sauvegardés dans '$(DIR_RES)/'."

# 9. Menu d'Aide
help:
	@echo "Utilisation : make [cible]"
	@echo "-------------------------------------------------------"
	@echo "  make             : Compile tout le projet C++"
	@echo "  make fe / img    : Compile un module spécifique"
	@echo "  make clean       : Supprime les fichiers .o"
	@echo "  make clean-data  : Vide les sorties de simulation (Sécurisé)"
	@echo "  make clean-legacy: Rase les anciens dossiers data pollués"
	@echo "  make fclean      : Clean + Clean-data"
	@echo "  make plot        : Lance les scripts Gnuplot"