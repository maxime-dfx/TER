#pragma once
#include <opencv2/opencv.hpp>

enum class FibreShape {
    CIRCLE,
    ELLIPSE,
    RECTANGLE
};

struct Fibre {
    cv::Point2f center;
    double radius;   // rayon (cercle) ou demi-diagonale (ellipse/rect)
    double rx;       // demi-grand axe ou demi-longueur
    double ry;       // demi-petit axe ou demi-largeur
    double angle;    // angle de rotation en radians
    FibreShape shape;

    // Constructeur cercle
    Fibre(cv::Point2f c, double r)
        : center(c), radius(r), rx(r), ry(r), angle(0.0), shape(FibreShape::CIRCLE) {}

    // Constructeur ellipse
    Fibre(cv::Point2f c, double rx_, double ry_, double angle_)
        : center(c), radius((rx_ + ry_) / 2.0),
          rx(rx_), ry(ry_), angle(angle_), shape(FibreShape::ELLIPSE) {}

    // Constructeur rectangle (memes parametres qu'ellipse, forme differente)
    static Fibre makeRect(cv::Point2f c, double halfLen, double halfWidth, double angle_) {
        Fibre f(c, halfLen, halfWidth, angle_);
        f.shape = FibreShape::RECTANGLE;
        return f;
    }
};
