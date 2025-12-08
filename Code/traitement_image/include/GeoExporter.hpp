#ifndef GEO_EXPORTER_HPP
#define GEO_EXPORTER_HPP

#include <vector>
#include <string>
#include "Fibre.hpp"

class GeoExporter {
public:
    // Constructeur : définit la taille du domaine (image)
    GeoExporter(int width, int height, double meshSize = 10.0);

    // Sauvegarde le fichier .geo
    bool save(const std::string& filename, const std::vector<Fibre>& fibres);

private:
    int m_width;
    int m_height;
    double m_lc; // Taille caractéristique du maillage (Mesh Size)
};

#endif