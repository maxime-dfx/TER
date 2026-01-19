#include "Datafile.h"
#include <stdexcept>
#include <iostream>

using namespace std;

// Constructeur
DataFile::DataFile(const string& filename) {
    try {
        // Lecture avec toml11
        m_config = toml::parse(filename);
    } catch (const std::exception& err) {
        cerr << "ERREUR FATALE : Impossible de lire le fichier de config '" << filename << "'\n"
             << "Raison : " << err.what() << endl;
        throw runtime_error("Echec lecture configuration");
    }
}

// --- Helper pour lire proprement sans crasher ---
template <typename T>
T get_val(const toml::value& conf, const string& section, const string& key, const T& def) {
    if (conf.contains(section)) {
        return toml::find_or<T>(conf.at(section), key, def);
    }
    return def;
}

// =========================================================
//                        IO
// =========================================================
string DataFile::getMeshFile() const {
    return get_val<string>(m_config, "io", "mesh_file", "data/maillage/default.mesh");
}

string DataFile::getOutputDir() const {
    return get_val<string>(m_config, "io", "output_dir", "data/resultats");
}

string DataFile::getOutputVtk() const {
    return get_val<string>(m_config, "io", "output_vtk", "resultat.vtk");
}

// =========================================================
//                     SIMULATION
// =========================================================
double DataFile::getStrainTarget() const {
    return get_val<double>(m_config, "simulation", "strain_target", 0.001);
}

bool DataFile::isPlaneStrain() const {
    return get_val<bool>(m_config, "simulation", "plane_strain", true);
}

// =========================================================
//                  MATERIAUX : MATRICE
// =========================================================
// Structure TOML attendue : [materiaux.matrice]
int DataFile::getMatriceLabel() const {
    if (m_config.contains("materiaux") && m_config.at("materiaux").contains("matrice")) {
        return toml::find_or<int>(m_config.at("materiaux").at("matrice"), "label", 101);
    }
    return 101; // Valeur par défaut
}

double DataFile::getMatriceE() const {
    if (m_config.contains("materiaux") && m_config.at("materiaux").contains("matrice")) {
        return toml::find_or<double>(m_config.at("materiaux").at("matrice"), "E", 3500.0);
    }
    return 3500.0;
}

double DataFile::getMatriceNu() const {
    if (m_config.contains("materiaux") && m_config.at("materiaux").contains("matrice")) {
        return toml::find_or<double>(m_config.at("materiaux").at("matrice"), "nu", 0.35);
    }
    return 0.35;
}

// =========================================================
//                   MATERIAUX : FIBRE
// =========================================================
// Structure TOML attendue : [materiaux.fibre]
int DataFile::getFibreLabel() const {
    if (m_config.contains("materiaux") && m_config.at("materiaux").contains("fibre")) {
        return toml::find_or<int>(m_config.at("materiaux").at("fibre"), "label", 102);
    }
    return 102;
}

double DataFile::getFibreE() const {
    if (m_config.contains("materiaux") && m_config.at("materiaux").contains("fibre")) {
        return toml::find_or<double>(m_config.at("materiaux").at("fibre"), "E", 280000.0);
    }
    return 280000.0;
}

double DataFile::getFibreNu() const {
    if (m_config.contains("materiaux") && m_config.at("materiaux").contains("fibre")) {
        return toml::find_or<double>(m_config.at("materiaux").at("fibre"), "nu", 0.20);
    }
    return 0.20;
}