#ifndef FIBRE_HPP
#define FIBRE_HPP

#include <opencv2/core.hpp>

// Structure simple pour représenter une fibre
struct Fibre {
    cv::Point2f center; // Position (x, y)
    double radius;      // Rayon
};

struct FibreEllipse {
    cv::Point2f center;
    double ray_a;
    double ray_b;
    float angle;
};
#endif