#include "AutoDetector.hpp"
#include <iostream>

using namespace cv;
using namespace std;

AutoDetector::AutoDetector(const string& imagePath) 
    : m_imagePath(imagePath) 
{
    // Chargement de l'image dès la construction
    m_imgRaw = imread(m_imagePath);
    if (m_imgRaw.empty()) {
        cerr << "ERREUR [AutoDetector]: Impossible de charger " << m_imagePath << endl;
    }

    // Valeurs par défaut (optimisées pour tes images précédentes)
    m_dp = 1.0;
    m_minDist = 50.0;
    m_param1 = 30.0;
    m_param2 = 20.0;
    m_minRadius = 70;
    m_maxRadius = 130;
}

void AutoDetector::setHoughParams(double minDist, double p1, double p2, int minR, int maxR) {
    m_minDist = minDist;
    m_param1 = p1;
    m_param2 = p2;
    m_minRadius = minR;
    m_maxRadius = maxR;
}

vector<Fibre> AutoDetector::run(bool showDebug) {
    if (m_imgRaw.empty()) return {};

    cout << "--- Detection Automatique (Hough) ---" << endl;

    // 1. Prétraitement
    Mat gray;
    if (m_imgRaw.channels() == 3) cvtColor(m_imgRaw, gray, COLOR_BGR2GRAY);
    else gray = m_imgRaw.clone();

    // Flou indispensable pour éviter les faux positifs dus au bruit
    GaussianBlur(gray, gray, Size(9, 9), 2, 2);

    // 2. Transformée de Hough
    vector<Vec3f> circles_hough;
    HoughCircles(gray, circles_hough, HOUGH_GRADIENT, m_dp, m_minDist,
                 m_param1, m_param2, m_minRadius, m_maxRadius);

    // 3. Conversion en objets Fibre
    vector<Fibre> detectedFibres;
    for (const auto& c : circles_hough) {
        // c[0]=x, c[1]=y, c[2]=radius
        detectedFibres.push_back({ Point2f(c[0], c[1]), (double)c[2] });
    }

    cout << "-> " << detectedFibres.size() << " fibres detectees." << endl;

    // 4. Visualisation (Optionnel)
    if (showDebug) {
        Mat debug = m_imgRaw.clone();
        for (const auto& f : detectedFibres) {
            // Cercle Vert
            circle(debug, f.center, (int)f.radius, Scalar(0, 255, 0), 2);
            // Centre Rouge
            circle(debug, f.center, 2, Scalar(0, 0, 255), 3);
        }

        // Redimensionner si l'image est énorme (pour qu'elle tienne à l'écran)
        if (debug.cols > 1200) {
            double scale = 1200.0 / debug.cols;
            resize(debug, debug, Size(), scale, scale);
        }

        imshow("Resultat AutoDetector (Appuyez sur une touche)", debug);
        waitKey(0);
        destroyWindow("Resultat AutoDetector (Appuyez sur une touche)");
    }

    return detectedFibres;
}