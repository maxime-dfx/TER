#ifndef BOUNDARYCONDITION_H
#define BOUNDARYCONDITION_H

#include <functional>

// Type de fonction qui prend (x,y) et retourne true si le noeud est concerné
using NodeSelector = std::function<bool(double x, double y)>;

struct BoundaryCondition {
    NodeSelector selector; // Le test géométrique
    int direction;         // 0 pour X, 1 pour Y
    double value;          // La valeur imposée (ex: 0.0 pour bloquer)
    
    // Constructeur pratique
    BoundaryCondition(NodeSelector sel, int dir, double val)
        : selector(sel), direction(dir), value(val) {}
};

#endif