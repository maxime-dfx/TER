#ifndef MANUAL_SELECTOR_HPP
#define MANUAL_SELECTOR_HPP

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include "Fibre.hpp"

class ManualSelector {
public:
    ManualSelector(const std::string& imagePath);
    
    // Lance la boucle de sélection. Retourne la liste des fibres.
    std::vector<Fibre> run();

private:
    // Méthodes internes
    static void mouseCallback(int event, int x, int y, int flags, void* userdata);
    void handleMouse(int event, int x, int y);
    void updateDisplay();

    // Données membres
    cv::Mat m_imgRaw;      // Image originale
    cv::Mat m_imgDisplay;  // Image affichée
    std::string m_windowName;
    
    std::vector<Fibre> m_fibres; // Liste des fibres validées
    
    // État de la sélection en cours
    bool m_isSelectingRadius;
    cv::Point2f m_tempCenter;
};

#endif