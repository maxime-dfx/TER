EXEC = ../bin/solver_fe
CXX = g++

# --- CONFIGURATION LOCALE (LINUX) ---
# On utilise HOME pour pointer directement vers ton dossier ~/librairies
# On pointe vers le dossier 'eigen' spécifique
EIGEN_DIR = $(HOME)/librairies/eigen

# Flags
# -I$(EIGEN_DIR) permet de trouver <Eigen/Sparse>
# -fopenmp est conservé pour le calcul parallèle
CXXFLAGS = -Wall -Wextra -std=c++17 -fopenmp -Iinclude -I$(EIGEN_DIR)

SRC = $(wildcard src/*.cpp)
OBJ = $(SRC:src/%.cpp=obj/%.o)

all: $(EXEC)

$(EXEC): $(OBJ)
	@mkdir -p ../bin
	$(CXX) $(CXXFLAGS) $(OBJ) -o $@

obj/%.o: src/%.cpp
	@mkdir -p obj
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf obj $(EXEC)