#ifndef POSTPROCESSOR_H
#define POSTPROCESSOR_H

#include "Mesh.h"
#include "Material.h" // Nécessaire pour recalculer les contraintes
#include <Eigen/Dense>
#include <string>
#include <vector>

class PostProcessor {
public:
    // On passe le MaterialManager pour calculer sigma = D * B * U
void exportToVTK(const std::string& filename, 
                 const std::vector<Node>& nodes, 
                 const std::vector<Element>& elements, 
                 const std::vector<double>& U_global);
};

#endif