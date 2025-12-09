#include "PostProcessor.h"
#include <fstream>
#include <iostream>
#include <iomanip>

using namespace std;

void PostProcessor::saveVTK(const string& filename, const Mesh& mesh, const Eigen::VectorXd& U) {
    ofstream out(filename);
    if (!out.is_open()) {
        cerr << "Erreur: Impossible d'écrire " << filename << endl;
        return;
    }

    // En-tête XML VTK
    out << "<?xml version=\"1.0\"?>\n";
    out << "<VTKFile type=\"UnstructuredGrid\" version=\"0.1\" byte_order=\"LittleEndian\">\n";
    out << "  <UnstructuredGrid>\n";
    out << "    <Piece NumberOfPoints=\"" << mesh.vertices.size() << "\" NumberOfCells=\"" << mesh.triangles.size() << "\">\n";

    // --- POINTS (Géométrie Initiale + Déformée optionnelle si on additionne U) ---
    // Ici on exporte la géométrie NON déformée, et on met U dans "PointData"
    // Paraview s'occupera de la déformation avec le filtre "Warp By Vector".
    out << "      <Points>\n";
    out << "        <DataArray type=\"Float32\" NumberOfComponents=\"3\" format=\"ascii\">\n";
    for (const auto& v : mesh.vertices) {
        out << v.x << " " << v.y << " 0.0\n";
    }
    out << "        </DataArray>\n";
    out << "      </Points>\n";

    // --- DONNEES AUX POINTS (Déplacements) ---
    out << "      <PointData Vectors=\"Displacement\">\n";
    out << "        <DataArray type=\"Float32\" Name=\"Displacement\" NumberOfComponents=\"3\" format=\"ascii\">\n";
    for (size_t i = 0; i < mesh.vertices.size(); ++i) {
        // U contient Ux, Uy. On ajoute 0.0 pour Uz
        out << U(2*i) << " " << U(2*i+1) << " 0.0\n";
    }
    out << "        </DataArray>\n";
    out << "      </PointData>\n";

    // --- CONNECTIVITE (Cellules) ---
    out << "      <Cells>\n";
    out << "        <DataArray type=\"Int32\" Name=\"connectivity\" format=\"ascii\">\n";
    for (const auto& tri : mesh.triangles) {
        out << tri.v[0] << " " << tri.v[1] << " " << tri.v[2] << "\n";
    }
    out << "        </DataArray>\n";
    
    out << "        <DataArray type=\"Int32\" Name=\"offsets\" format=\"ascii\">\n";
    for (size_t i = 0; i < mesh.triangles.size(); ++i) {
        out << (i + 1) * 3 << "\n";
    }
    out << "        </DataArray>\n";
    
    out << "        <DataArray type=\"UInt8\" Name=\"types\" format=\"ascii\">\n";
    for (size_t i = 0; i < mesh.triangles.size(); ++i) {
        out << "5\n"; // 5 = VTK_TRIANGLE
    }
    out << "        </DataArray>\n";
    out << "      </Cells>\n";

    out << "    </Piece>\n";
    out << "  </UnstructuredGrid>\n";
    out << "</VTKFile>\n";
}

void PostProcessor::saveAnimationSeries(const string& baseName, const Mesh& mesh, const Eigen::VectorXd& U_final, int frameCount, double visualAmp) {
    cout << "Generation animation (" << frameCount << " frames)..." << endl;
    
    for (int i = 0; i <= frameCount; ++i) {
        double t = (double)i / frameCount;
        
        // On crée un déplacement interpolé U_t = U_final * t * amplification
        Eigen::VectorXd U_t = U_final * t * visualAmp;

        string filename = baseName + "_" + to_string(i) + ".vtu";
        saveVTK(filename, mesh, U_t);
    }
    cout << "-> Export termine." << endl;
}