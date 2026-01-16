EXEC = ../bin/traitement_img
CXX = g++

# --- CONFIGURATION OPENCV LOCALE (LINUX) ---
# On utilise HOME pour pointer vers ton dossier perso
OPENCV_ROOT = $(HOME)/librairies/opencv_install

# Chemin des includes (headers)
OPENCV_INC = -I$(OPENCV_ROOT)/include/opencv4

# Chemin des librairies (libs)
# Sous Linux, les fichiers compilés sont dans 'lib' (pas x64/mingw)
OPENCV_LIB_DIR = -L$(OPENCV_ROOT)/lib64

# Liste des modules à lier (Sans suffixe de version sous Linux)
OPENCV_LIBS = -lopencv_core \
              -lopencv_imgproc \
              -lopencv_highgui \
              -lopencv_imgcodecs

# --- FIN CONFIGURATION ---

CXXFLAGS = -Wall -Wextra -std=c++11 -Iinclude $(OPENCV_INC)
# Important : -Wl,-rpath inscrit le chemin des libs DANS l'exécutable
LDFLAGS = $(OPENCV_LIB_DIR) $(OPENCV_LIBS) -Wl,-rpath,$(OPENCV_ROOT)/lib

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
	rm -rf obj $(EXEC)