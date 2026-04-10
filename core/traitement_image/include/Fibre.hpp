#ifndef FIBRE_HPP
#define FIBRE_HPP

#include <opencv2/core.hpp>

// Structure simple pour représenter une fibre
struct Fibre {
    cv::Point2f center; // Position (x, y)
    double radius;      // Rayon
};

#endif