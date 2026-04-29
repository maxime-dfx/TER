#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include "Fibre.h"

class AutoDetector {
public:
    AutoDetector(const std::string& imagePath);

    // Parametres optionnels avances
    void setMinRadius(int r) { m_minRadius = r; }
    void setMaxRadius(int r) { m_maxRadius = r; }
    void setMinVotes(int v)  { m_minVotes = v; }

    // Lance la detection
    std::vector<Fibre> run(bool showDebug = false);

private:
    std::string m_imagePath;
    cv::Mat m_imgRaw;
    int m_minRadius;
    int m_maxRadius;
    int m_minVotes;

    // Preprocessing
    cv::Mat preprocess(const cv::Mat& gray);

    // NMS : supprime les cercles trop proches
    std::vector<cv::Vec3f> nms(const std::vector<cv::Vec3f>& circles, double overlapThresh);
};
