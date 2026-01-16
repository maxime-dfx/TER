#ifndef MESH_H
#define MESH_H

#include <vector>
#include <string>
#include <cmath>

// Structure pour la Bounding Box (Boîte englobante)
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
public:
    std::vector<Vertex> vertices;
    std::vector<Triangle> triangles;

    Mesh() = default;

    void read(const std::string& filename);
    void printStats() const;

    // --- NOUVELLES MÉTHODES ---
    // Calcule l'aire d'un triangle spécifique (pour Solver & PostPro)
    double getTriangleArea(int triangleIndex) const;
    
    // Calcule les dimensions totales du modèle
    BoundingBox getBounds() const;
};

#endif