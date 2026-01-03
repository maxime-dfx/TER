EXEC = ../bin/solver_fe
CXX = g++

# --- CHEMIN EIGEN CORRIGÉ ---
# On remonte de 'finite_elements' vers 'Code' (..), puis vers 'TER' (..) pour trouver 'librairies'
EIGEN_DIR = ../../librairies

# Flags
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
	rm -rf obj ../bin/solver_fe.exe