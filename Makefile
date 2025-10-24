all : exec


exec : COO.cpp Solver.cpp main.cpp
	clear
	g++ -Wall -std=c++11 -I Eigen/eigen -o run COO.cpp Solver.cpp main.cpp
	#Compilation réussi
	./run

zip :
	tar czvf '../TER_2A.tgz' ../'2A'
