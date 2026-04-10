#include <iostream>
#include <string>
#include <vector>
#include <cstdlib> // Pour system()
#include <opencv2/opencv.hpp>

// Nos modules
#include "Fibre.hpp"
#include "ManualSelector.hpp"
#include "AutoDetector.hpp"
#include "GeoExporter.hpp"

using namespace std;

int main(int argc, char** argv) {
    // 1. Configuration
    string imgPath = (argc > 1) ? argv[1] : "data/images_raw/1.2.png";
    string geoPath = "data/geo/maillage.geo";
    string debugPath = "data/images_segmentées/debug_overlay.png";

    cout << "=========================================" << endl;
    cout << "   GENERATEUR DE MAILLAGE FIBRES (FEM)   " << endl;
    cout << "=========================================" << endl;
    cout << "Image cible : " << imgPath << endl;

    // Menu utilisateur
    cout << "Mode de detection ?" << endl;
    cout << "  [m] Manuel      (Clic souris)" << endl;
    cout << "  [a] Automatique (Algorithme)" << endl;
    cout << "Votre choix : ";
    char choix;
    cin >> choix;

    vector<Fibre> fibres;

    // 2. Exécution selon le choix
    if (choix == 'a' || choix == 'A') {
        AutoDetector detector(imgPath);
        fibres = detector.run(false); 
    } 
    else if (choix == 'm' || choix == 'M') {
        ManualSelector selector(imgPath);
        fibres = selector.run();
    } 
    else {
        cout << "Choix inconnu." << endl;
        return 0;
    }

    // Sécurité : Si aucune fibre trouvée
    if (fibres.empty()) {
        cout << "Aucune fibre trouvee. Arrêt." << endl;
        return 0;
    }

    // Chargement de l'image originale
    cv::Mat img = cv::imread(imgPath);
    if (img.empty()) {
        cerr << "Erreur : Impossible de lire l'image originale." << endl;
        return -1;
    }

    // ---------------------------------------------------------
    // ETAPE 3 : Création de l'image de contrôle (Overlay)
    // ---------------------------------------------------------
    cv::Mat overlay = img.clone();

    for (const auto& f : fibres) {
        // --- CORRECTION ICI (Accès via center.x / center.y / radius) ---
        cv::Point center((int)f.center.x, (int)f.center.y);
        int radius = (int)f.radius;

        // 1. Dessiner le contour du cercle en VERT (BGR: 0, 255, 0)
        cv::circle(overlay, center, radius, cv::Scalar(0, 255, 0), 2);

        // 2. Dessiner le centre en ROUGE (BGR: 0, 0, 255)
        cv::circle(overlay, center, 2, cv::Scalar(0, 0, 255), -1);
    }

    // Sauvegarde
    cv::imwrite(debugPath, overlay);
    cout << "-----------------------------------------" << endl;
    cout << "[CONTROLE] Image de superposition générée : " << debugPath << endl;


    // ---------------------------------------------------------
    // ETAPE 4 : Exportation Géométrie (.geo)
    // ---------------------------------------------------------
    GeoExporter exporter(img.cols, img.rows, 20.0);

    if (exporter.save(geoPath, fibres)) {
        cout << "[ETAPE 1] Géométrie générée : " << geoPath << endl;

        // ---------------------------------------------------------
        // ETAPE 5 : Conversion automatique en .mesh via Gmsh
        // ---------------------------------------------------------
        cout << "[ETAPE 2] Maillage en cours (Conversion .geo -> .mesh)..." << endl;
        
        string meshPath = "data/maillage/maillage.mesh";
        string cmd = "gmsh " + geoPath + " -2 -format mesh -o " + meshPath + " -v 2";
        
        int result = system(cmd.c_str());

        if (result == 0) {
            cout << "SUCCES ! Fichier final prêt : " << meshPath << endl;
        } else {
            cerr << "ERREUR : Gmsh n'a pas pu générer le maillage." << endl;
        }
        cout << "-----------------------------------------" << endl;
    }

    return 0;
}