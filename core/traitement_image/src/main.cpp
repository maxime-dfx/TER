#include <iostream>
#include <string>
#include <vector>
#include <fstream> // Ajout pour écrire dans un fichier
#include "AutoDetector.h"
#include "ManualSelector.h"
// On a supprimé #include "GeoExporter.h" !

using namespace std;

int main(int argc, char** argv) {
    if (argc < 2) {
        cout << "Usage: ./traitement_img <chemin_image> [manuel/auto] [fichier_sortie.csv]" << endl;
        return 1;
    }

    string imagePath = argv[1];
    string mode = (argc > 2) ? argv[2] : "auto";
    string outputPath = (argc > 3) ? argv[3] : "fibres.csv";

    cout << "=== Analyse d'Image Composite ===" << endl;
    cout << "Image : " << imagePath << " | Mode : " << mode << endl;

    vector<Fibre> fibres;

    if (mode == "manuel") {
        ManualSelector selector(imagePath);
        fibres = selector.run();
    } else {
        AutoDetector detector(imagePath);
        fibres = detector.run(true); 
    }

    if (fibres.empty()) {
        cout << "[-] Aucune fibre detectee ou selectionnee." << endl;
        return 1;
    }
    
    ofstream outFile(outputPath);
    if (!outFile.is_open()) {
        cerr << "[-] Erreur : Impossible de creer le fichier " << outputPath << endl;
        return 1;
    }

    for (const auto& f : fibres) {
        outFile << f.center.x << "," << f.center.y << "," << f.radius << "\n";
    }
    
    outFile.close();
    cout << "=== Analyse terminee avec succes (" << fibres.size() << " fibres) ===" << endl;
    
    return 0;
}