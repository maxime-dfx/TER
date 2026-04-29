#ifndef BOUNDARYCONDITION_H
#define BOUNDARYCONDITION_H

#include <functional>

// Type de fonction qui prend (x,y) et retourne true si le noeud est concerné
using NodeSelector = std::function<bool(double x, double y)>;
using ValueFunction = std::function<double(double, double)>;

struct BoundaryCondition {
    NodeSelector selector;
    int direction;
    ValueFunction valueFunc;

    BoundaryCondition(NodeSelector s, int d, ValueFunction v) 
        : selector(s), direction(d), valueFunc(v) {}
};

struct PeriodicCondition {
    int nodeSlave;   // ID du nœud sur le bord esclave (ex: Droite)
    int nodeMaster;  // ID du nœud sur le bord maître (ex: Gauche)
    int direction;   // 0 pour X, 1 pour Y
    double delta;    // Le saut de déplacement imposé (ex: Lx * strain)
    
    PeriodicCondition(int s, int m, int dir, double d)
        : nodeSlave(s), nodeMaster(m), direction(dir), delta(d) {}
};

#endif