#pragma once
#include <string>
#include <vector>
#include "Fibre.h"

class GeoExporter {
public:
    GeoExporter(int width, int height, double meshSize = 20.0);

    // Sauvegarde le fichier .geo (cercles et ellipses)
    bool save(const std::string& filename, const std::vector<Fibre>& fibres);

private:
    int    m_width;
    int    m_height;
    double m_lc;
};
