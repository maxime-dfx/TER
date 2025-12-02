#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <vector>

// Fonction écriture polygones
int write_geo_polygons(const std::string& filename, const std::vector<std::vector<cv::Point>>& contours, int height, const std::string& label) {
    std::ofstream geo(filename);
    if(!geo.is_open()) return -1;

    geo << "// Gmsh " << label << " (Contours Lisses)\n"; 
    geo << "lc = 1.0;\n"; 
    int pointID = 1, lineID = 1, surfaceID = 1;

    for(const auto& c : contours) {
        if(c.size() < 3) continue;
        std::vector<cv::Point> poly;
        // Augmentation de l'epsilon pour un lissage plus prononcé
        cv::approxPolyDP(c, poly, 2.0, true); // epsilon passé de 1.5 à 2.0

        std::vector<int> ptsIDs;
        for(const auto& p : poly) {
            geo << "Point(" << pointID << ") = {" << p.x << "," << (height - p.y) << ",0, lc};\n";
            ptsIDs.push_back(pointID++);
        }
        std::vector<int> lineIDs;
        for(size_t i = 0; i < ptsIDs.size(); i++) {
            geo << "Line(" << lineID << ") = {" << ptsIDs[i] << "," << ptsIDs[(i + 1) % ptsIDs.size()] << "};\n";
            lineIDs.push_back(lineID++);
        }
        geo << "Curve Loop(" << surfaceID << ") = {";
        for(size_t i = 0; i < lineIDs.size(); i++) {
            geo << lineIDs[i]; if(i < lineIDs.size() - 1) geo << ",";
        }
        geo << "};\nPlane Surface(" << surfaceID << ") = {" << surfaceID << "};\n";
        surfaceID++;
    }
    geo.close();
    return 0;
}

int main() 
{
    std::string imagePath = "data/images_raw/1.png"; 
    cv::Mat image = cv::imread(imagePath);
    if(image.empty()) { std::cerr << "Erreur image.\n"; return -1; }

    cv::Mat gris;
    cv::cvtColor(image, gris, cv::COLOR_BGR2GRAY);

    // 1. PRETRAITEMENT (Flou légèrement plus fort)
    cv::Mat gris_lissee;
    cv::medianBlur(gris, gris_lissee, 9); // Flou passé de 7 à 9

    // 2. SEUILLAGE (Vos valeurs validées)
    int SEUIL_VIDE = 35;    
    int SEUIL_FIBRE_MIN = 100;
    int SEUIL_FIBRE_MAX = 123;

    cv::Mat mask_vide, mask_fibres, mask_matrice;

    // A. VIDE
    cv::inRange(gris_lissee, 0, SEUIL_VIDE, mask_vide);

    // B. FIBRES
    cv::inRange(gris_lissee, SEUIL_FIBRE_MIN, SEUIL_FIBRE_MAX, mask_fibres);

    // C. MATRICE (Le reste)
    cv::Mat occupied;
    cv::bitwise_or(mask_vide, mask_fibres, occupied);
    cv::bitwise_not(occupied, mask_matrice);


    // 3. NETTOYAGE MORPHOLOGIQUE (Affinements)
    
    // Noyaux de différentes tailles
    cv::Mat k_small = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3,3));
    cv::Mat k_medium = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5,5));
    cv::Mat k_large = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(7,7)); 

    // Nettoyage Vide
    cv::morphologyEx(mask_vide, mask_vide, cv::MORPH_OPEN, k_medium);

    // Nettoyage Fibres (Open pour la séparation, Close pour boucher les petits trous internes)
    // OPEN (6x6) et CLOSE (4x4) pour plus de flexibilité
    cv::morphologyEx(mask_fibres, mask_fibres, cv::MORPH_OPEN, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(6,6))); 
    cv::morphologyEx(mask_fibres, mask_fibres, cv::MORPH_CLOSE, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(4,4)));

    // Nettoyage Matrice (OPEN pour le bruit, CLOSE pour souder)
    cv::morphologyEx(mask_matrice, mask_matrice, cv::MORPH_OPEN, k_small);
    cv::morphologyEx(mask_matrice, mask_matrice, cv::MORPH_CLOSE, k_large); // CLOSE fort pour la continuité


    // 4. EXTRACTION
    std::vector<std::vector<cv::Point>> c_vide, c_matrice, c_fibres;
    
    cv::findContours(mask_vide, c_vide, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    cv::findContours(mask_fibres, c_fibres, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    cv::findContours(mask_matrice, c_matrice, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

    // 5. FILTRAGE ET EXPORT (Ajustement des seuils d'aire)
    std::vector<std::vector<cv::Point>> c_vide_f, c_matrice_f, c_fibres_f;

    for(auto& c : c_vide) if(cv::contourArea(c) > 50) c_vide_f.push_back(c);
    for(auto& c : c_matrice) if(cv::contourArea(c) > 100) c_matrice_f.push_back(c); // Légèrement réduit pour les petits interstices
    for(auto& c : c_fibres) if(cv::contourArea(c) > 400) c_fibres_f.push_back(c); // Laissez-moi réduire un peu, certaines fibres sont plus petites

    write_geo_polygons("data/geo/vide.geo", c_vide_f, image.rows, "Vide");
    write_geo_polygons("data/geo/matrice.geo", c_matrice_f, image.rows, "Matrice");
    write_geo_polygons("data/geo/fibres.geo", c_fibres_f, image.rows, "Fibres");

    // 6. IMAGE CONTROLE
    cv::Mat result = image.clone();
    cv::drawContours(result, c_vide_f, -1, cv::Scalar(255, 0, 0), 2);    // Bleu = Vide
    cv::drawContours(result, c_matrice_f, -1, cv::Scalar(0, 255, 0), 1); // Vert = Matrice
    cv::drawContours(result, c_fibres_f, -1, cv::Scalar(0, 0, 255), 2);  // Rouge = Fibre

    std::string out = "data/images_segmenté/superposition_v21_final_lissage.png";
    cv::imwrite(out, result);
    std::cout << "Verifiez : " << out << std::endl;

    return 0;
}