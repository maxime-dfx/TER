#define _USE_MATH_DEFINES
#include "GeoExporter.h"
#include <fstream>
#include <iostream>
#include <cmath>
#include <algorithm>

using namespace std;

GeoExporter::GeoExporter(int width, int height, double meshSize)
    : m_width(width), m_height(height), m_lc(meshSize) {}

bool GeoExporter::save(const string& filename, const vector<Fibre>& fibres) {
    ofstream f(filename);
    if (!f.is_open()) {
        cerr << "Erreur : Impossible d'ouvrir " << filename << endl;
        return false;
    }

    int nC = 0, nE = 0, nR = 0;
    for (const auto& fib : fibres) {
        if      (fib.shape == FibreShape::CIRCLE)    nC++;
        else if (fib.shape == FibreShape::ELLIPSE)   nE++;
        else if (fib.shape == FibreShape::RECTANGLE) nR++;
    }
    cout << "Generation .geo : " << nC << " cercles, "
         << nE << " ellipses, " << nR << " rectangles..." << endl;

    f << "// Fichier genere automatiquement\n";
    f << "SetFactory(\"OpenCASCADE\");\n\n";

    f << "Mesh.SaveAll = 0;\n";
    f << "Mesh.CharacteristicLengthMax = " << m_lc << ";\n";
    f << "Mesh.CharacteristicLengthMin = " << (m_lc / 8.0) << ";\n";
    f << "Geometry.Tolerance = 1e-6;\n\n";

    // Domaine englobant (la matrice initiale)
    f << "Rectangle(1) = {0, 0, 0, " << m_width << ", " << m_height << "};\n\n";

    // Outil de clipping (meme dimensions, pour intersecter les fibres au domaine)
    f << "Rectangle(2) = {0, 0, 0, " << m_width << ", " << m_height << "};\n\n";

    f << "fibres_finales[] = {};\n\n";

    int start_id = 1000;

    for (size_t i = 0; i < fibres.size(); ++i) {
        const Fibre& fib = fibres[i];
        int id = start_id + (int)i;
        double fy = m_height - fib.center.y;  // Y inverse (image -> repere math)

        if (fib.shape == FibreShape::CIRCLE) {
            f << "Disk(" << id << ") = {"
              << fib.center.x << ", " << fy << ", 0, " << fib.radius << "};\n";
        }
        else if (fib.shape == FibreShape::ELLIPSE) {
            double rx = fib.rx, ry = fib.ry;
            if (rx < ry) swap(rx, ry);
            f << "Disk(" << id << ") = {"
              << fib.center.x << ", " << fy << ", 0, " << rx << ", " << ry << "};\n";
            if (fabs(fib.angle) > 0.01)
                f << "Rotate {{0,0,1},{" << fib.center.x << "," << fy << ",0},"
                  << fib.angle << "} { Surface{" << id << "}; }\n";
        }
        else if (fib.shape == FibreShape::RECTANGLE) {
            f << "Rectangle(" << id << ") = {"
              << (fib.center.x - fib.rx) << ", " << (fy - fib.ry) << ", 0, "
              << (2.0 * fib.rx) << ", " << (2.0 * fib.ry) << "};\n";
            if (fabs(fib.angle) > 0.01)
                f << "Rotate {{0,0,1},{" << fib.center.x << "," << fy << ",0},"
                  << fib.angle << "} { Surface{" << id << "}; }\n";
        }

        // Clipper la fibre au domaine (intersection avec Rectangle(2))
        f << "temp[] = BooleanIntersection{ Surface{" << id
          << "}; Delete; }{ Surface{2}; };\n";
        f << "fibres_finales[] += temp[];\n\n";
    }

    // Supprimer l'outil de clipping
    f << "Recursive Delete { Surface{2}; }\n\n";

    // ================================================================
    // APPROCHE ROBUSTE : BooleanDifference
    // Matrice = domaine (Surface{1}) MOINS toutes les fibres
    // Les fibres conservent leurs tags (pas de Delete sur l'outil)
    // ================================================================
    f << "// Matrice = domaine - fibres\n";
    f << "mat[] = BooleanDifference{ Surface{1}; Delete; }{ Surface{fibres_finales[]}; };\n";
    f << "Coherence;\n\n";

    // Physical Groups : identification directe, aucune ambiguite
    f << "Physical Surface(\"Matrice\", 101) = mat[];\n";
    f << "If (#fibres_finales[] > 0)\n";
    f << "  Physical Surface(\"Fibres\", 102) = fibres_finales[];\n";
    f << "EndIf\n";

    f.close();
    cout << "Fichier .geo genere : " << filename << endl;
    return true;
}