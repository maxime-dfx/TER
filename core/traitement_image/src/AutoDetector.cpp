#include "AutoDetector.h"
#include <iostream>

using namespace cv;
using namespace std;

AutoDetector::AutoDetector(const string& imagePath)
    : m_imagePath(imagePath), m_minRadius(10), m_maxRadius(200), m_minVotes(2)
{
    m_imgRaw = imread(m_imagePath);
    if (m_imgRaw.empty()) {
        cerr << "ERREUR [AutoDetector]: Impossible de charger " << m_imagePath << endl;
    }
}

vector<Fibre> AutoDetector::run(bool showDebug) {
    if (m_imgRaw.empty()) return {};

    cout << "--- Detection Automatique : Transformee de Distance (Maxima Locaux) ---" << endl;

    // =========================================================================
    // ⚙️ LE BOUTON MAGIQUE :
    // Si tes cercles se dessinent dans la résine au lieu des fibres, 
    // passe ce booléen à 'true'.
    bool INVERSER_COULEURS = false; 
    // =========================================================================

    Mat gray;
    if (m_imgRaw.channels() == 3) cvtColor(m_imgRaw, gray, COLOR_BGR2GRAY);
    else gray = m_imgRaw.clone();

    // 1. Amélioration du contraste (CLAHE)
    Ptr<CLAHE> clahe = createCLAHE(2.0, Size(8, 8));
    clahe->apply(gray, gray);

    // 2. Binarisation (Otsu)
    Mat thresh;
    if (!INVERSER_COULEURS) {
        threshold(gray, thresh, 0, 255, THRESH_BINARY | THRESH_OTSU);
    } else {
        threshold(gray, thresh, 0, 255, THRESH_BINARY_INV | THRESH_OTSU);
    }

    // 3. Nettoyage morphologique
    // On efface le bruit et on lisse les bords des fibres
    Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(3, 3));
    morphologyEx(thresh, thresh, MORPH_OPEN, kernel, Point(-1, -1), 2);

    // 4. Transformée de distance (Crée des montagnes au centre des zones blanches)
    Mat dist;
    distanceTransform(thresh, dist, DIST_L2, 3);

    // 5. LA CORRECTION : Recherche des Maxima Locaux (Sommets des montagnes)
    // Au lieu d'un seuil global, on dilate l'image. 
    // Si un pixel est égal à son voisinage dilaté, c'est le sommet d'une fibre !
    Mat dist_dilate;
    dilate(dist, dist_dilate, Mat::ones(5, 5, CV_8U)); // Fenêtre 5x5 pour éviter les micro-pics
    
    // On garde le sommet SI il est un maximum local ET si la distance est > à la moitié du rayon min
    Mat sure_fg = (dist == dist_dilate) & (dist > (m_minRadius * 0.5f));
    sure_fg.convertTo(sure_fg, CV_8U, 255);

    // 6. Extraction des coordonnées exactes
    vector<vector<Point>> contours;
    findContours(sure_fg, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    vector<Fibre> detectedFibres;
    for (const auto& contour : contours) {
        // Moments = Centre de gravité exact du sommet trouvé
        Moments m = moments(contour);
        if (m.m00 == 0) continue;
        int cx = int(m.m10 / m.m00);
        int cy = int(m.m01 / m.m00);

        if (cx >= 0 && cx < dist.cols && cy >= 0 && cy < dist.rows) {
            float realRadius = dist.at<float>(cy, cx);

            // On filtre selon tes tolérances
            if (realRadius >= m_minRadius && realRadius <= m_maxRadius) {
                detectedFibres.push_back(Fibre(Point2f(cx, cy), (double)realRadius));
            }
        }
    }

    cout << "   -> Fibres detectees et centrees : " << detectedFibres.size() << endl;

    // 7. Affichage visuel (Validation)
    if (showDebug) {
        Mat debug = m_imgRaw.clone();
        for (const auto& f : detectedFibres) {
            circle(debug, f.center, (int)f.radius, Scalar(0, 255, 0), 2); // Contour vert
            circle(debug, f.center, 2, Scalar(0, 0, 255), -1);            // Centre rouge
        }
        
        if (debug.cols > 1000) {
            double scale = 1000.0 / debug.cols;
            resize(debug, debug, Size(), scale, scale);
            resize(thresh, thresh, Size(), scale, scale);
        }
        
        // FENÊTRE CRITIQUE : Ce qui est BLANC ici = L'intérieur des fibres pour l'algorithme
        imshow("Etape 0 : Binarisation (Les fibres DOIVENT etre blanches !)", thresh);
        imshow("Resultat Final", debug);
        waitKey(0);
        destroyAllWindows();
    }

    return detectedFibres;
}