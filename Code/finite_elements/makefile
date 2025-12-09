EXEC = ../bin/solver_fe
CXX = g++
# Adaptez ce chemin si votre version d'Eigen est différente
EIGEN_DIR = ../../Librairie/libs/Eigen/eigen-3.4.0

# AJOUT DE -fopenmp ICI
CXXFLAGS = -Wall -Wextra -std=c++11 -fopenmp -Iinclude -I$(EIGEN_DIR)

SRC = $(wildcard src/*.cpp)
OBJ = $(SRC:src/%.cpp=obj/%.o)

all: $(EXEC)

$(EXEC): $(OBJ)
	@mkdir -p ../bin
	# AJOUT DE $(CXXFLAGS) ICI pour passer -fopenmp au linker
	$(CXX) $(CXXFLAGS) $(OBJ) -o $@

obj/%.o: src/%.cpp
	@mkdir -p obj
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf obj