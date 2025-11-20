all : test


test : COO.cpp Solver.cpp maillage.cpp z_fonctions.cpp main.cpp
	clear
	g++ -Wall -std=c++11 -I Eigen/eigen -o run COO.cpp Solver.cpp maillage.cpp z_fonctions.cpp main_test.cpp
	#Compilation réussi
	./run

	
exec : COO.cpp Solver.cpp maillage.cpp z_fonctions.cpp main.cpp
	clear
	g++ -Wall -std=c++11 -I Eigen/eigen -o run COO.cpp Solver.cpp maillage.cpp z_fonctions.cpp main.cpp
	#Compilation réussi
	./run

zip :
	tar czvf '../TER_2A.tgz' ../'2A'
