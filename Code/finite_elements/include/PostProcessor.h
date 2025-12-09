#ifndef POSTPROCESSOR_H
#define POSTPROCESSOR_H

#include "Mesh.h"
#include <Eigen/Dense>
#include <string>
#include <vector>

class PostProcessor {
public:
    // Exporte une seule frame au format VTK XML (.vtu)
    static void saveVTK(const std::string& filename, const Mesh& mesh, const Eigen::VectorXd& U);

    // Génère une série temporelle (animation)
    // frameCount : nombre d'images
    // totalDeformation : facteur d'amplification visuelle (ex: 1.0 ou 100.0)
    static void saveAnimationSeries(const std::string& baseName, const Mesh& mesh, const Eigen::VectorXd& U, int frameCount, double visualAmp);
};

#endif