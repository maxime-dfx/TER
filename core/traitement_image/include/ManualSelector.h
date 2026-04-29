#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include "Fibre.h"

enum class SelectionMode {
    CIRCLE,
    ELLIPSE,
    RECTANGLE
};

class ManualSelector {
public:
    ManualSelector(const std::string& imagePath);
    std::vector<Fibre> run();

private:
    static void mouseCallback(int event, int x, int y, int flags, void* userdata);
    void handleMouse(int event, int x, int y);
    void updateDisplay();
    void printHelp();

    std::string    m_windowName;
    cv::Mat        m_imgRaw;
    cv::Mat        m_imgDisplay;
    std::vector<Fibre> m_fibres;

    SelectionMode  m_mode;
    bool           m_step1Done;   // centre pose
    bool           m_step2Done;   // grand axe pose (pour ellipse/rect : etape 3 = petit axe)
    cv::Point2f    m_tempCenter;
    cv::Point2f    m_tempAxisEnd; // bout du grand axe
};
