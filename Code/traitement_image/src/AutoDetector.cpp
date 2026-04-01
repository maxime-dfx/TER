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

// vector<FibreEllipse> AutoDetector::run_ellipse(bool showDebug) {
//     if (m_imgRaw.empty()) return {};

//     cout << "--- Detection Automatique ellipses ---" << endl;

//     // 1. Prétraitement
//     Mat gray;
//     if (m_imgRaw.channels() == 3) cvtColor(m_imgRaw, gray, COLOR_BGR2GRAY);
//     else gray = m_imgRaw.clone();

//     // Flou indispensable pour éviter les faux positifs dus au bruit
//     GaussianBlur(gray, gray, Size(9, 9), 2, 2);


//     Mat edges;
//     adaptiveThreshold(gray,edges,50,150);


//     vector<vector<Point>> contours;
//     findContours(edges, contours, RETR_LIST, CHAIN_APPROX_NONE);



//     vector<RotatedRect> ellipses;

//     for (const auto& contour : contours) {
//         if (contour.size() < 50) continue; // important !

//         RotatedRect ellipse = fitEllipse(contour);
//         ellipses.push_back(ellipse);
//     }
//     // // 2. Transformée de Hough
//     // vector<Vec3f> circles_hough;
//     // HoughCircles(gray, circles_hough, HOUGH_GRADIENT, m_dp, m_minDist,
//     //              m_param1, m_param2, m_minRadius, m_maxRadius);

//     // 3. Conversion en objets Fibre
//     // vector<FibreEllipse> detectedFibres;
//     // for (const auto& c : ellipses) {
//     //     // c[0]=x, c[1]=y, c[2]=radius
//     //     detectedFibres.push_back({ Point2f(c[0], c[1]), (double)c[2] , (double)c[3] });
//     // }

//     vector<FibreEllipse> detectedFibres;

//     for (const auto& e : ellipses) 
//     {
//         FibreEllipse f;
//         f.center = e.center;
//         f.ray_a = (double)e.size.width / 2.0;
//         f.ray_b = (double)e.size.height / 2.0;
//         f.angle = (double)e.angle;

//         detectedFibres.push_back(f);
//     }

//     cout << "-> " << detectedFibres.size() << " fibres detectees." << endl;

//     // 4. Visualisation (Optionnel)
//     if (showDebug) {
//         Mat debug = m_imgRaw.clone();
//         // for (const auto& f : detectedFibres) {
//         //     // Cercle Vert
//         //     circle(debug, f.center, (int)f.radius, Scalar(0, 255, 0), 2);
//         //     // Centre Rouge
//         //     circle(debug, f.center, 2, Scalar(0, 0, 255), 3);
//         // }

//         for (const auto& f : detectedFibres)
//         {
//             // Ellipse verte
//             ellipse(debug,
//                     f.center,
//                     Size(f.ray_a, f.ray_b), // demi-axes
//                     f.angle,
//                     0, 360,
//                     Scalar(0, 255, 0),
//                     2);

//             // Centre rouge
//             circle(debug, f.center, 2, Scalar(0, 0, 255), 3);
//         }

//         // Redimensionner si l'image est énorme (pour qu'elle tienne à l'écran)
//         if (debug.cols > 1200) {
//             double scale = 1200.0 / debug.cols;
//             resize(debug, debug, Size(), scale, scale);
//         }

//         imshow("Resultat AutoDetector (Appuyez sur une touche)", debug);
//         waitKey(0);
//         destroyWindow("Resultat AutoDetector (Appuyez sur une touche)");
//     }

//     return detectedFibres;
// }



vector<FibreEllipse> AutoDetector::run_ellipse(bool showDebug) {
    if (m_imgRaw.empty()) return {};

    //D'ici a :
    Mat binary , hsv;
    cvtColor(m_imgRaw, hsv , COLOR_BGR2GRAY);

    cv::Scalar coul_min(80,10,39.5*2.55);
    cv::Scalar coul_max(180,30,41*2.55);

    cv::inRange(hsv,coul_min,coul_max,binary);
     // 1. Lissage important pour fusionner la texture interne des fibres
    // Augmentez le Sigma (le '3') si c'est encore trop bruité
    GaussianBlur(binary, binary, Size(9, 9), 4);

    // 2. Utilisation de OTSU (calcule le seuil auto)
    threshold(binary, binary, 0, 255, THRESH_BINARY | THRESH_OTSU);

    // 3. Morphologie pour nettoyer
    // OPEN : enlève les petits points blancs isolés
    // CLOSE : bouche les trous dans les fibres
    Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(4, 4));
    morphologyEx(binary, binary, MORPH_OPEN, kernel);
    morphologyEx(binary, binary, MORPH_CLOSE, kernel);
    //ici
    imwrite("verifs/debug_inrange.png", binary);

    // Ici c'est un traitement fonctionnel 'oetsu' mais on tente autre chose :

    // Mat gray, binary;
    // if (m_imgRaw.channels() == 3) cvtColor(m_imgRaw, gray, COLOR_BGR2GRAY);
    // else gray = m_imgRaw.clone();

    // // 1. Lissage important pour fusionner la texture interne des fibres
    // // Augmentez le Sigma (le '3') si c'est encore trop bruité
    // GaussianBlur(gray, gray, Size(9, 9), 4);

    // // 2. Utilisation de OTSU (calcule le seuil auto)
    // threshold(gray, binary, 0, 255, THRESH_BINARY | THRESH_OTSU);

    // // 3. Morphologie pour nettoyer
    // // OPEN : enlève les petits points blancs isolés
    // // CLOSE : bouche les trous dans les fibres
    // Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(4, 4));
    // morphologyEx(binary, binary, MORPH_OPEN, kernel);
    // morphologyEx(binary, binary, MORPH_CLOSE, kernel);

    // // Diagnostic : vérifiez ce fichier !
    // imwrite("verifs/debug_otsu.png", binary);

    //Jusque la


    // 4. Trouver les contours
    vector<vector<Point>> contours;
    findContours(binary, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    
    // --- DEBUG PAR FICHIER ---
    // On enregistre l'image binaire pour voir ce que l'algo "voit"
    cv::imwrite("verifs/debug_binarisation.png", binary);

    // On peut aussi enregistrer le résultat avec les contours dessinés
    Mat debugResult = m_imgRaw.clone();
    drawContours(debugResult, contours, -1, Scalar(0, 255, 0), 2);
    cv::imwrite("verifs/debug_contours.png", debugResult);

    cout << "Images de debug enregistrees sous 'debug_binarisation.png' et 'debug_contours.png'" << endl;
    // --------------------------


    double min, max;
    min = 2500000 ;
    max = min;
    vector<FibreEllipse> detectedFibres;
    for (const auto& contour : contours) {
        // Filtrage plus souple : au moins 15 points pour définir une ellipse
        if (contour.size() < 50)
        {
            //cout<<"contour trop petit ==> skip"<<endl;
            continue;
        } 

        // Calcul de l'aire pour éliminer les poussières
        double area = contourArea(contour);
        
        if (area < 23000 || area > 250000) // À ajuster selon la taille réelle de vos fibres
        {
            //cout<<"Pas assez d'aire ==> skip"<<endl;
            continue; 
        }
        else if (area < min)
        {
            min = area;
        }
        else if (area > max)
        {
            max = area;
        }
        // cout<<"Area = "<<area<<endl;

        // Fit ellipse
        RotatedRect e = fitEllipse(contour);

        // Optionnel : Vérifier la "circularité" (ratio largeur/hauteur)
        // Pour éviter de détecter des lignes allongées qui ne sont pas des fibres
        float ratio = e.size.width / e.size.height;
        if (ratio < 0.2 || ratio > 5.0)
        {
            //cout<<"pas bon ratio ==> skip"<<endl;
            continue;
        } 
        //if (ratio < 0.5 || ratio > 1.5) continue;

        FibreEllipse f;
        f.center = e.center;
        f.ray_a = (double)e.size.width / 2.0;
        f.ray_b = (double)e.size.height / 2.0;
        f.angle = (double)e.angle;
        detectedFibres.push_back(f);
        //cout<<"Ajout d'une fibre"<<endl;
    }
    cout<<"max : "<<max<<" et min : "<<min<<endl;
    return detectedFibres;
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


    // // 2. Transformée de Hough
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