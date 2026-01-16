#include "Mesh.h"
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>

using namespace std;

void Mesh::read(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) throw runtime_error("Impossible d'ouvrir " + filename);

    string keyword;
    int dim = 2;

    while (file >> keyword) {
        if (keyword == "Dimension") {
            file >> dim;
        }
        else if (keyword == "Vertices") {
            int nb; file >> nb;
            vertices.reserve(nb);
            for (int i = 0; i < nb; ++i) {
                double x, y, z_dummy; int lbl;
                if (dim == 3) file >> x >> y >> z_dummy >> lbl;
                else          file >> x >> y >> lbl;
                vertices.push_back({x, y, lbl});
            }
        }
        else if (keyword == "Triangles") {
            int nb; file >> nb;
            triangles.reserve(nb);
            for (int i = 0; i < nb; ++i) {
                int n1, n2, n3, ref;
                file >> n1 >> n2 >> n3 >> ref;
                triangles.push_back({n1-1, n2-1, n3-1, ref}); // Indices 0-based
            }
        }
        else if (keyword == "End") break;
    }
    
    if(vertices.empty()) throw runtime_error("Maillage vide ou mal formé !");
    cout << "[Mesh] " << vertices.size() << " noeuds, " << triangles.size() << " elements." << endl;
}

double Mesh::getTriangleArea(int i) const {
    const Triangle& t = triangles[i];
    const Vertex& p0 = vertices[t.v[0]];
    const Vertex& p1 = vertices[t.v[1]];
    const Vertex& p2 = vertices[t.v[2]];
    return 0.5 * std::abs((p1.x - p0.x)*(p2.y - p0.y) - (p2.x - p0.x)*(p1.y - p0.y));
}

BoundingBox Mesh::getBounds() const {
    BoundingBox bb = {1e9, -1e9, 1e9, -1e9, 0, 0};
    for (const auto& v : vertices) {
        if(v.x < bb.xmin) bb.xmin = v.x;
        if(v.x > bb.xmax) bb.xmax = v.x;
        if(v.y < bb.ymin) bb.ymin = v.y;
        if(v.y > bb.ymax) bb.ymax = v.y;
    }
    bb.L = bb.xmax - bb.xmin;
    bb.H = bb.ymax - bb.ymin;
    return bb;
}

void Mesh::printStats() const {
    auto bb = getBounds();
    cout << "   -> Dimensions : " << bb.L << " x " << bb.H << endl;
}