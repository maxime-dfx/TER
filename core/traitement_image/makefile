EXEC = ../bin/traitement_img.exe
CXX = C:/msys64/mingw64/bin/g++

# --- CONFIGURATION OPENCV LOCALE (WINDOWS MSYS2) ---
# Chemin des includes (headers)
OPENCV_INC = -I C:/msys64/mingw64/include/opencv4

# Chemin du dossier contenant les librairies (.a / .dll.a)
OPENCV_LIB_DIR = -L C:/msys64/mingw64/lib

# Liste des modules OpenCV à lier
OPENCV_LIBS = -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_imgcodecs

# --- OPTIONS DE COMPILATION ---
CXXFLAGS = -Wall -Wextra -std=c++17 -Iinclude -Isrc $(OPENCV_INC)
# On a retiré le rpath (inutile sur Windows)
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
	rm -rf obj $(EXEC)