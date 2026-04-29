EXEC = ../bin/solver_fe
CXX = g++

# --- CONFIGURATION LOCALE ---
EIGEN_DIR = $(HOME)/librairies/eigen
TOML_DIR = $(HOME)/librairies/toml11/include

# Flags de compilation
CXXFLAGS = -Wall -Wextra -std=c++17 -fopenmp -Iinclude -static -static-libgcc -static-libstdc++ -I$(EIGEN_DIR) -I$(TOML_DIR)

# --- NOUVEAU : Flags d'édition de liens pour l'autonomie ---
LDFLAGS = -static -static-libgcc -static-libstdc++ -Wl,-Bstatic -lstdc++ -lpthread -lgomp -Wl,-Bdynamic

SRC = $(wildcard src/# ==========================================
# CONFIGURATION DU COMPILATEUR
# ==========================================
CXX = g++
EXEC = ../bin/solver_fe.exe

# --- CHEMINS DES LIBRAIRIES ---
# Utilisation de ?= pour permettre de surcharger ces chemins en ligne de commande
EIGEN_DIR ?= $(HOME)/librairies/eigen
TOML_DIR  ?= $(HOME)/librairies/toml11/include

# ==========================================
# FLAGS DE COMPILATION & ÉDITION DES LIENS
# ==========================================

# -Iinclude : tes propres headers
# -fopenmp  : indispensable pour la parallélisation de ton solveur
CXXFLAGS = -O3 -Wall -Wextra -std=c++17 -fopenmp \
           -Iinclude -I$(EIGEN_DIR) -I$(TOML_DIR)

# --- CONFIGURATION STATIQUE (Solution au Code 3221225781) ---
# On force l'inclusion des librairies standards dans l'exécutable
# pour qu'il soit autonome et n'ait pas besoin de DLL externes.
LDFLAGS = -static -static-libgcc -static-libstdc++ \
          -Wl,-Bstatic -lstdc++ -lpthread -lgomp -Wl,-Bdynamic

# ==========================================
# GESTION DES FICHIERS SOURCE ET OBJETS
# ==========================================
SRC = $(wildcard src/*.cpp)
OBJ = $(SRC:src/%.cpp=obj/%.o)

# ==========================================
# RÈGLES DE COMPILATION
# ==========================================

.PHONY: all clean directories

all: directories $(EXEC)

# Création de l'exécutable final
$(EXEC): $(OBJ)
	@echo "--- Édition des liens : $(EXEC) ---"
	$(CXX) $(OBJ) -o $@ $(CXXFLAGS) $(LDFLAGS)
	@echo "--- Compilation terminée avec succès ! ---"

# Compilation des fichiers objets (.cpp -> .o)
obj/%.o: src/%.cpp
	@echo "    Compilation de $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Création des dossiers nécessaires
directories:
	@mkdir -p obj
	@mkdir -p ../bin

# Nettoyage
clean:
	@echo "Nettoyage des fichiers objets et de l'exécutable..."
	rm -rf obj $(EXEC)*.cpp)
OBJ = $(SRC:src/%.cpp=obj/%.o)

all: $(EXEC)

$(EXEC): $(OBJ)
	@mkdir -p ../bin
	$(CXX) $(CXXFLAGS) $(OBJ) -o $@ $(LDFLAGS)

obj/%.o: src/%.cpp
	@mkdir -p obj
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf obj $(EXEC)