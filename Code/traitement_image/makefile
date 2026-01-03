EXEC = ../bin/traitement_img
CXX = g++

# --- CONFIGURATION OPENCV LOCALE ---
OPENCV_ROOT = ../librairies/opencv_install

# Chemin des includes (souvent dans include/opencv4)
OPENCV_INC = -I$(OPENCV_ROOT)/include/opencv4

# Chemin des librairies (libs)
# Sous MinGW, c'est souvent dans x64/mingw/lib. Si cela échoue, essayez juste /lib
OPENCV_LIB_DIR = -L$(OPENCV_ROOT)/x64/mingw/lib

# Version du suffixe des fichiers .a (ex: libopencv_core4100.dll.a -> 4100)
# Regardez dans le dossier lib pour confirmer ce numéro.
OPENCV_VER = 4100

# Liste des modules à lier
OPENCV_LIBS = -lopencv_core$(OPENCV_VER) \
              -lopencv_imgproc$(OPENCV_VER) \
              -lopencv_highgui$(OPENCV_VER) \
              -lopencv_imgcodecs$(OPENCV_VER)

# --- FIN CONFIGURATION ---

CXXFLAGS = -Wall -Wextra -std=c++11 -Iinclude $(OPENCV_INC)
LDFLAGS = $(OPENCV_LIB_DIR) $(OPENCV_LIBS)

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
	rm -rf obj ../bin/traitement_img.exe