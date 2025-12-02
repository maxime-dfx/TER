EXEC = ../bin/traitement_img
CXX = g++

# --- CONFIGURATION OPENCV LOCALE ---
OPENCV_ROOT = ../libs/opencv_install
OPENCV_INC = -I$(OPENCV_ROOT)/include/opencv4
OPENCV_LIB_DIR = -L$(OPENCV_ROOT)/lib64
OPENCV_LIBS = -lopencv_core -lopencv_imgproc -lopencv_imgcodecs -lopencv_highgui

CXXFLAGS = -Wall -Wextra -std=c++11 -Iinclude $(OPENCV_INC)
LDFLAGS = $(OPENCV_LIB_DIR) $(OPENCV_LIBS)
# -----------------------------------

SRC = $(wildcard src/*.cpp)
OBJ = $(SRC:src/%.cpp=obj/%.o)

all: $(EXEC)

$(EXEC): $(OBJ)
	@mkdir -p ../bin
	$(CXX) $(OBJ) -o $@ $(LDFLAGS)
	# Astuce pour que Linux trouve la librairie au lancement
	@patchelf --set-rpath '$$ORIGIN/../libs/opencv_install/lib64' $@ 2>/dev/null || true

obj/%.o: src/%.cpp
	@mkdir -p obj
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf obj