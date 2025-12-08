EXEC = ../bin/traitement_img
CXX = g++

# Ajout de -Iinclude pour que g++ trouve tes fichiers .hpp
CXXFLAGS = -Wall -Wextra -std=c++11 -Iinclude $(shell pkg-config --cflags opencv4)
LDFLAGS = $(shell pkg-config --libs opencv4)

SRC = $(wildcard src/*.cpp)
OBJ = $(SRC:src/%.cpp=obj/%.o)

all: $(EXEC)

$(EXEC): $(OBJ)
	@mkdir -p ../bin
	$(CXX) $(OBJ) -o $@ $(LDFLAGS)

obj/%.o: src/%.cpp
	@mkdir -p obj
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf obj ../bin/traitement_img