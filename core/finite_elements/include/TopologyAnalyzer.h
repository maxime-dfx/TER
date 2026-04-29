#ifndef TOPOLOGY_ANALYZER_H
#define TOPOLOGY_ANALYZER_H

#include <vector>
#include <utility>
#include "Mesh.h"

class TopologyAnalyzer {
private:
    const Mesh& mesh;
    double tolerance;
    bool periodic;

    // Listes des indices des noeuds sur les bords
    std::vector<int> nodesLeft;
    std::vector<int> nodesRight;
    std::vector<int> nodesBottom;
    std::vector<int> nodesTop;

    // Paires de noeuds pour les conditions périodiques (maître, esclave)
    std::vector<std::pair<int, int>> pairsLR;
    std::vector<std::pair<int, int>> pairsBT;

public:
    // Le constructeur prend une référence au maillage et un facteur de tolérance optionnel
    TopologyAnalyzer(const Mesh& m, double tolFactor = 1e-5);

    // Lance l'analyse topologique
    void analyze();

    // Accesseurs
    bool isPeriodic() const { return periodic; }

    const std::vector<int>& getNodesLeft() const { return nodesLeft; }
    const std::vector<int>& getNodesRight() const { return nodesRight; }
    const std::vector<int>& getNodesBottom() const { return nodesBottom; }
    const std::vector<int>& getNodesTop() const { return nodesTop; }

    const std::vector<std::pair<int, int>>& getPairsLR() const { return pairsLR; }
    const std::vector<std::pair<int, int>>& getPairsBT() const { return pairsBT; }
};

#endif // TOPOLOGY_ANALYZER_H²