#include "ManualSelector.hpp"
#include <iostream>

using namespace cv;
using namespace std;

ManualSelector::ManualSelector(const string& imagePath) 
    : m_windowName("Selection Manuelle"), m_isSelectingRadius(false) 
{
    m_imgRaw = imread(imagePath);
    if (m_imgRaw.empty()) {
        cerr << "Erreur: Impossible de charger " << imagePath << endl;
        exit(-1);
    }
    m_imgDisplay = m_imgRaw.clone();
}

// Fonction statique requise par OpenCV pour le callback
void ManualSelector::mouseCallback(int event, int x, int y, int flags, void* userdata) {
    ManualSelector* self = static_cast<ManualSelector*>(userdata);
    self->handleMouse(event, x, y);
}

void ManualSelector::handleMouse(int event, int x, int y) {
    if (event == EVENT_LBUTTONDOWN) {
        if (!m_isSelectingRadius) {
            // Étape 1 : Clic Centre
            m_tempCenter = Point2f((float)x, (float)y);
            m_isSelectingRadius = true;
            cout << "Centre: (" << x << "," << y << ") -> Clique le rayon." << endl;
        } else {
            // Étape 2 : Clic Rayon
            double r = norm(Point2f((float)x, (float)y) - m_tempCenter);
            m_fibres.push_back({m_tempCenter, r});
            m_isSelectingRadius = false;
            cout << "Fibre ajoutee : R=" << r << endl;
            updateDisplay();
        }
    } else if (event == EVENT_RBUTTONDOWN) {
        // Clic Droit : Annuler
        if (!m_fibres.empty()) {
            m_fibres.pop_back();
            m_isSelectingRadius = false;
            cout << "Derniere fibre supprimee." << endl;
            updateDisplay();
        }
    } else if (event == EVENT_MOUSEMOVE && m_isSelectingRadius) {
        // Optionnel : Prévisualisation dynamique
        Mat temp = m_imgDisplay.clone();
        double r = norm(Point2f((float)x, (float)y) - m_tempCenter);
        circle(temp, m_tempCenter, (int)r, Scalar(0, 165, 255), 1); // Orange
        imshow(m_windowName, temp);
        return; // On return pour ne pas écraser l'affichage par updateDisplay()
    }
    
    // Si on a cliqué, on rafraichit l'affichage statique
    if (event == EVENT_LBUTTONDOWN || event == EVENT_RBUTTONDOWN) {
        updateDisplay();
    }
}

void ManualSelector::updateDisplay() {
    m_imgDisplay = m_imgRaw.clone();
    
    // Dessiner toutes les fibres validées
    for (const auto& f : m_fibres) {
        circle(m_imgDisplay, f.center, (int)f.radius, Scalar(0, 255, 0), 2); // Vert
        circle(m_imgDisplay, f.center, 2, Scalar(0, 0, 255), -1); // Point rouge
    }

    // Dessiner le centre en cours si on attend le rayon
    if (m_isSelectingRadius) {
        circle(m_imgDisplay, m_tempCenter, 2, Scalar(0, 0, 255), -1);
    }

    imshow(m_windowName, m_imgDisplay);
}

vector<Fibre> ManualSelector::run() {
    namedWindow(m_windowName, WINDOW_AUTOSIZE);
    // On passe 'this' pour que la fonction statique puisse accéder à l'objet
    setMouseCallback(m_windowName, mouseCallback, this);

    cout << "=== MODE SELECTION ===" << endl;
    cout << "[Clic Gauche] 1x Centre, 1x Bord" << endl;
    cout << "[Clic Droit]  Annuler dernier" << endl;
    cout << "[ESPACE]      Terminer et Sauvegarder" << endl;

    updateDisplay();

    while (true) {
        char k = (char)waitKey(10);
        if (k == ' ' || k == 27 || k == 's') break; // Espace, ESC ou 's'
    }
    
    destroyWindow(m_windowName);
    return m_fibres;
}