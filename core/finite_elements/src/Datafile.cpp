#include "Datafile.h"
#include <stdexcept>
#include <iostream>

using namespace std;

DataFile::DataFile(const string& filename) {
    try {
        m_config = toml::parse(filename);
    } catch (const std::exception& err) {
        throw runtime_error(string("Echec lecture configuration '") + filename + "' : " + err.what());
    }
}

template <typename T>
T get_val(const toml::value& conf, const string& section, const string& key, const T& def) {
    if (conf.contains(section)) {
        return toml::find_or<T>(conf.at(section), key, def);
    }
    return def;
}

// ==========================================
// THÈME 1 : [io] (Entrées / Sorties)
// ==========================================
string DataFile::getMeshFile() const { return get_val<string>(m_config, "io", "mesh_file", "data/maillage/default.mesh"); }
string DataFile::getOutputDir() const { return get_val<string>(m_config, "io", "output_dir", "data/resultats"); }
string DataFile::getOutputVtk() const { return get_val<string>(m_config, "io", "output_vtk", "resultat.vtk"); }


// ==========================================
// THÈME 2 : [simulation] (Paramètres de calcul)
// ==========================================
string DataFile::getLoadCase() const { return get_val<string>(m_config, "simulation", "load_case", "traction_x"); }
bool DataFile::isPlaneStrain() const { return get_val<bool>(m_config, "simulation", "plane_strain", true); }
double DataFile::getStrainTarget() const { return get_val<double>(m_config, "simulation", "strain_target", 0.001); }
double DataFile::getHalpinTsaiXi() const { return get_val<double>(m_config, "simulation", "halpin_tsai_xi", 2.0); }
double DataFile::getDeltaT() const { return get_val<double>(m_config, "simulation", "delta_T", 0.0); }
int DataFile::getLoadSteps() const { return get_val<int>(m_config, "simulation", "load_steps", 10); }


// ==========================================
// THÈME 3 : [materiaux] (Propriétés physiques)
// ==========================================

// --- Sous-thème : Matrice ---
int DataFile::getMatriceLabel() const {
    if (m_config.contains("materiaux") && m_config.at("materiaux").contains("matrice"))
        return toml::find_or<int>(m_config.at("materiaux").at("matrice"), "label", 1);
    return 1;
}
double DataFile::getMatriceE() const {
    if (m_config.contains("materiaux") && m_config.at("materiaux").contains("matrice"))
        return toml::find_or<double>(m_config.at("materiaux").at("matrice"), "E", 3500.0);
    return 3500.0;
}
double DataFile::getMatriceNu() const {
    if (m_config.contains("materiaux") && m_config.at("materiaux").contains("matrice"))
        return toml::find_or<double>(m_config.at("materiaux").at("matrice"), "nu", 0.35);
    return 0.35;
}
double DataFile::getMatriceAlpha() const {
    if (m_config.contains("materiaux") && m_config.at("materiaux").contains("matrice"))
        return toml::find_or<double>(m_config.at("materiaux").at("matrice"), "alpha", 60e-6);
    return 60e-6; 
}
double DataFile::getMatriceXt() const {
    if (m_config.contains("materiaux") && m_config.at("materiaux").contains("matrice"))
        return toml::find_or<double>(m_config.at("materiaux").at("matrice"), "Xt", 80.0); 
    return 80.0;
}
double DataFile::getMatriceXc() const {
    if (m_config.contains("materiaux") && m_config.at("materiaux").contains("matrice"))
        return toml::find_or<double>(m_config.at("materiaux").at("matrice"), "Xc", 80.0); 
    return 80.0;
}
double DataFile::getMatriceG_Ic() const { return get_val<double>(m_config, "simulation", "G_Ic", 0.0); }


// --- Sous-thème : Fibre ---
int DataFile::getFibreLabel() const {
    if (m_config.contains("materiaux") && m_config.at("materiaux").contains("fibre"))
        return toml::find_or<int>(m_config.at("materiaux").at("fibre"), "label", 2);
    return 2;
}
double DataFile::getFibreE() const {
    if (m_config.contains("materiaux") && m_config.at("materiaux").contains("fibre"))
        return toml::find_or<double>(m_config.at("materiaux").at("fibre"), "E", 280000.0);
    return 280000.0;
}
double DataFile::getFibreNu() const {
    if (m_config.contains("materiaux") && m_config.at("materiaux").contains("fibre"))
        return toml::find_or<double>(m_config.at("materiaux").at("fibre"), "nu", 0.20);
    return 0.20;
}
double DataFile::getFibreAlpha() const {
    if (m_config.contains("materiaux") && m_config.at("materiaux").contains("fibre"))
        return toml::find_or<double>(m_config.at("materiaux").at("fibre"), "alpha", 15e-6);
    return 15e-6; 
}
double DataFile::getFibreXt() const {
    if (m_config.contains("materiaux") && m_config.at("materiaux").contains("fibre"))
        return toml::find_or<double>(m_config.at("materiaux").at("fibre"), "Xt", 2500.0); 
    return 2500.0;
}
double DataFile::getFibreXc() const {
    if (m_config.contains("materiaux") && m_config.at("materiaux").contains("fibre"))
        return toml::find_or<double>(m_config.at("materiaux").at("fibre"), "Xc", 2500.0); 
    return 2500.0;
}
double DataFile::getFibreG_Ic() const { return get_val<double>(m_config, "simulation", "G_Ic", 0.0); }


// --- Sous-thème : Interphase ---
int DataFile::getInterphaseLabel() const {
    if (m_config.contains("materiaux") && m_config.at("materiaux").contains("interphase"))
        return toml::find_or<int>(m_config.at("materiaux").at("interphase"), "label", 3);
    return 3;
}
double DataFile::getInterphaseE() const {
    if (m_config.contains("materiaux") && m_config.at("materiaux").contains("interphase"))
        return toml::find_or<double>(m_config.at("materiaux").at("interphase"), "E", 2000.0);
    return 2000.0;
}
double DataFile::getInterphaseNu() const {
    if (m_config.contains("materiaux") && m_config.at("materiaux").contains("interphase"))
        return toml::find_or<double>(m_config.at("materiaux").at("interphase"), "nu", 0.38);
    return 0.38;
}
double DataFile::getInterphaseAlpha() const {
    if (m_config.contains("materiaux") && m_config.at("materiaux").contains("interphase"))
        return toml::find_or<double>(m_config.at("materiaux").at("interphase"), "alpha", 70e-6);
    return 70e-6; 
}
double DataFile::getInterphaseXt() const {
    if (m_config.contains("materiaux") && m_config.at("materiaux").contains("interphase"))
        return toml::find_or<double>(m_config.at("materiaux").at("interphase"), "Xt", 40.0);
    return 40.0;
}
double DataFile::getInterphaseXc() const {
    if (m_config.contains("materiaux") && m_config.at("materiaux").contains("interphase"))
        return toml::find_or<double>(m_config.at("materiaux").at("interphase"), "Xc", 100.0);
    return 100.0;
}
double DataFile::getInterphaseG_Ic() const { return get_val<double>(m_config, "simulation", "G_Ic", 0.0); }
