#ifndef MESH_H
#define MESH_H

#include <vector>
#include <string>
#include <cmath>
#include <Eigen/Dense>

struct BoundingBox {
    double xmin, xmax, ymin, ymax;
    double L, H;
};

struct Vertex {
    double x, y;
    int label; 
};

struct Triangle {
    int v[3];  
    int label; 
};

class Mesh {
private:
    std::vector<Vertex> vertices;
    std::vector<Triangle> triangles;

public:
    Mesh() = default;

    void read(const std::string& filename);
    void printStats() const;

    // Accesseurs constants (Encapsulation)
    const std::vector<Vertex>& getVertices() const { return vertices; }
    const std::vector<Triangle>& getTriangles() const { return triangles; }

    // Calcule l'aire d'un triangle spécifique
    double getTriangleArea(int triangleIndex) const;
    
    // Calcule les dimensions totales du modèle
    BoundingBox getBounds() const;
    Eigen::Matrix<double, 3, 6> computeBMatrix(int triangleIndex, double& area) const;
};

#endif