#include "TopologyAnalyzer.h"
#include <cmath>
#include <iostream>
#include <algorithm>

using namespace std;

TopologyAnalyzer::TopologyAnalyzer(const Mesh& m, double tolFactor) 
    : mesh(m), periodic(false) {
    auto bb = mesh.getBounds();
    // Calcul de la tolérance absolue basée sur les dimensions de la boîte englobante
    double Lx = bb.L; 
    double Ly = bb.H; 
    tolerance = tolFactor * max(Lx, Ly);
}

void TopologyAnalyzer::analyze() {
    auto bb = mesh.getBounds();
    const auto& verts = mesh.getVertices();

    // Réinitialisation en cas d'appels multiples
    nodesLeft.clear(); nodesRight.clear();
    nodesBottom.clear(); nodesTop.clear();
    pairsLR.clear(); pairsBT.clear();

    // 1. Identification des noeuds sur les bords du Volume Élémentaire Représentatif (VER)
    for (size_t i = 0; i < verts.size(); ++i) {
        if (abs(verts[i].x - bb.xmin) < tolerance) nodesLeft.push_back(i);
        if (abs(verts[i].x - bb.xmax) < tolerance) nodesRight.push_back(i);
        if (abs(verts[i].y - bb.ymin) < tolerance) nodesBottom.push_back(i);
        if (abs(verts[i].y - bb.ymax) < tolerance) nodesTop.push_back(i);
    }

    // 2. Appairage Gauche/Droite (Y identiques)
    for (int m : nodesLeft) {
        for (int s : nodesRight) {
            if (abs(verts[m].y - verts[s].y) < tolerance) {
                pairsLR.push_back({m, s}); 
                break; // Dès qu'on trouve l'esclave, on passe au maître suivant
            }
        }
    }

    // 3. Appairage Bas/Haut (X identiques)
    for (int m : nodesBottom) {
        for (int s : nodesTop) {
            if (abs(verts[m].x - verts[s].x) < tolerance) {
                pairsBT.push_back({m, s}); 
                break;
            }
        }
    }

    // 4. Bilan de la périodicité
    periodic = (pairsLR.size() == nodesLeft.size() && 
                pairsBT.size() == nodesBottom.size() && 
                !nodesLeft.empty());
}