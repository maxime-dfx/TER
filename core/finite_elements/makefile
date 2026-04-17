EXEC = ../bin/solver_fe
CXX = g++

# --- CONFIGURATION LOCALE ---
EIGEN_DIR = $(HOME)/librairies/eigen
TOML_DIR = $(HOME)/librairies/toml11/include

# Flags de compilation
CXXFLAGS = -Wall -Wextra -std=c++17 -fopenmp -Iinclude -I$(EIGEN_DIR) -I$(TOML_DIR)

# --- NOUVEAU : Flags d'édition de liens pour l'autonomie ---
LDFLAGS = -static -static-libgcc -static-libstdc++ -Wl,-Bstatic -lstdc++ -lpthread -Wl,-Bdynamic

SRC = $(wildcard src/*.cpp)
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