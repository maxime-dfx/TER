#ifndef AUTODETECTOR_HPP
#define AUTODETECTOR_HPP

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include "Fibre.hpp"

class AutoDetector {
public:
    // Constructeur : charge l'image
    AutoDetector(const std::string& imagePath);

    // Lance la détection et retourne les fibres trouvées
    // showDebug : si true, ouvre une fenêtre pour valider visuellement
    std::vector<Fibre> run(bool showDebug = true);

    // Permet de modifier les paramètres sans recompiler la classe (Setters)
    void setHoughParams(double minDist, double p1, double p2, int minR, int maxR);

private:
    std::string m_imagePath;
    cv::Mat m_imgRaw;

    // Paramètres de Hough
    double m_dp;
    double m_minDist;
    double m_param1;
    double m_param2;
    int m_minRadius;
    int m_maxRadius;
};

#endif