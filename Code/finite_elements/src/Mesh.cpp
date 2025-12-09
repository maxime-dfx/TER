#include "Mesh.h"
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <algorithm>

using namespace std;

Mesh::Mesh() {
    // Constructeur par défaut
}

void Mesh::read(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        throw runtime_error("Erreur : Impossible d'ouvrir le fichier " + filename);
    }

    string keyword;
    int dim = 2;

    while (file >> keyword) {
        
        // [1] DIMENSION
        if (keyword == "Dimension") {
            file >> dim;
            if(dim != 2 && dim != 3) {
                cerr << "[Attention] Dimension " << dim << " détectée, seule la 2D est supportée." << endl;
            }
        }

        // [2] VERTICES
        else if (keyword == "Vertices") {
            int nbVertices;
            file >> nbVertices;
            
            if(nbVertices <= 0) {
                throw runtime_error("Nombre de sommets invalide: " + to_string(nbVertices));
            }
            
            vertices.reserve(nbVertices);

            for (int i = 0; i < nbVertices; ++i) {
                double x, y, z_dummy;
                int label;

                if (dim == 3) {
                    file >> x >> y >> z_dummy >> label;
                } else {
                    file >> x >> y >> label;
                }
                
                if(file.fail()) {
                    throw runtime_error("Erreur de lecture au sommet " + to_string(i+1));
                }

                vertices.push_back({x, y, label});
            }
        }
        
        // [3] TRIANGLES
        else if (keyword == "Triangles") {
            int nbTriangles;
            file >> nbTriangles;
            
            if(nbTriangles <= 0) {
                throw runtime_error("Nombre de triangles invalide: " + to_string(nbTriangles));
            }
            
            triangles.reserve(nbTriangles);

            for (int i = 0; i < nbTriangles; ++i) {
                int n1, n2, n3, ref;
                file >> n1 >> n2 >> n3 >> ref;
                
                if(file.fail()) {
                    throw runtime_error("Erreur de lecture au triangle " + to_string(i+1));
                }

                // Conversion base-1 vers base-0
                n1--; n2--; n3--;
                
                // Validation des indices
                int maxIdx = (int)vertices.size();
                if(n1 < 0 || n1 >= maxIdx || 
                   n2 < 0 || n2 >= maxIdx || 
                   n3 < 0 || n3 >= maxIdx) {
                    throw runtime_error(
                        "Triangle " + to_string(i+1) + " : indices hors limites [" +
                        to_string(n1) + ", " + to_string(n2) + ", " + to_string(n3) + 
                        "], max=" + to_string(maxIdx-1)
                    );
                }

                Triangle tri;
                tri.v[0] = n1;
                tri.v[1] = n2;
                tri.v[2] = n3;
                tri.label = ref;
                triangles.push_back(tri);
            }
        }
        
        // [4] FIN
        else if (keyword == "End") {
            break;
        }
        
        // Ignorer les autres blocs (Edges, etc.)
    }
    
    file.close();
    
    // Validation finale
    if(vertices.empty()) {
        throw runtime_error("Aucun sommet lu dans le fichier!");
    }
    if(triangles.empty()) {
        throw runtime_error("Aucun triangle lu dans le fichier!");
    }
    
    cout << "[Mesh] Lecture terminée : " << vertices.size() << " sommets, " 
         << triangles.size() << " triangles (Dimension: " << dim << ")." << endl;
}

void Mesh::printStats() const {
    cout << "=== Statistiques du Maillage ===" << endl;
    cout << "Nombre de sommets   : " << vertices.size() << endl;
    cout << "Nombre de triangles : " << triangles.size() << endl;
    
    if (!vertices.empty()) {
        cout << "Premier sommet      : (" << vertices[0].x << ", " 
             << vertices[0].y << "), label=" << vertices[0].label << endl;
        cout << "Dernier sommet      : (" << vertices.back().x << ", " 
             << vertices.back().y << "), label=" << vertices.back().label << endl;
    }
    
    if (!triangles.empty()) {
        // Statistiques sur les labels de triangles
        int min_label = triangles[0].label;
        int max_label = triangles[0].label;
        for(const auto& tri : triangles) {
            min_label = min(min_label, tri.label);
            max_label = max(max_label, tri.label);
        }
        cout << "Labels triangles    : [" << min_label << ", " << max_label << "]" << endl;
    }
    
    cout << "================================" << endl;
}