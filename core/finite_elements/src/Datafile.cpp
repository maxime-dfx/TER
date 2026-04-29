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

// Helper générique pour les sections simples ([io], [simulation])
template <typename T>
T get_val(const toml::value& conf, const string& section, const string& key, const T& def) {
    if (conf.contains(section)) {
        return toml::find_or<T>(conf.at(section), key, def);
    }
    return def;
}

// Helper spécifique pour les tables imbriquées ([materiaux.matrice], etc.)
template <typename T>
T get_mat_val(const toml::value& conf, const string& mat_name, const string& key, const T& def) {
    if (conf.contains("materiaux") && conf.at("materiaux").contains(mat_name)) {
        return toml::find_or<T>(conf.at("materiaux").at(mat_name), key, def);
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

// Nouveaux paramètres Haute Fidélité corrigés
bool DataFile::includeCuringStresses() const { return get_val<bool>(m_config, "simulation", "include_curing_stresses", false); }
double DataFile::getCuringDeltaT() const { return get_val<double>(m_config, "simulation", "curing_delta_T", 0.0); }


// ==========================================
// THÈME 3 : [materiaux] (Propriétés physiques)
// ==========================================

// --- Sous-thème : Matrice ---
int DataFile::getMatriceLabel() const { return get_mat_val<int>(m_config, "matrice", "label", 1); }
double DataFile::getMatriceE() const     { return get_mat_val<double>(m_config, "matrice", "E", 3500.0); }
double DataFile::getMatriceNu() const    { return get_mat_val<double>(m_config, "matrice", "nu", 0.35); }
double DataFile::getMatriceAlpha() const { return get_mat_val<double>(m_config, "matrice", "alpha", 60e-6); }
double DataFile::getMatriceXt() const    { return get_mat_val<double>(m_config, "matrice", "Xt", 80.0); }
double DataFile::getMatriceXc() const    { return get_mat_val<double>(m_config, "matrice", "Xc", 80.0); }
double DataFile::getMatriceG_Ic() const  { return get_mat_val<double>(m_config, "matrice", "GIc", 150.0); }

// --- Sous-thème : Fibre ---
int DataFile::getFibreLabel() const { return get_mat_val<int>(m_config, "fibre", "label", 2); }
double DataFile::getFibreE() const     { return get_mat_val<double>(m_config, "fibre", "E", 230000.0); }
double DataFile::getFibreNu() const    { return get_mat_val<double>(m_config, "fibre", "nu", 0.20); }
double DataFile::getFibreAlpha() const { return get_mat_val<double>(m_config, "fibre", "alpha", -1e-6); }
double DataFile::getFibreXt() const    { return get_mat_val<double>(m_config, "fibre", "Xt", 3000.0); }
double DataFile::getFibreXc() const    { return get_mat_val<double>(m_config, "fibre", "Xc", 2000.0); }
double DataFile::getFibreG_Ic() const  { return get_mat_val<double>(m_config, "fibre", "GIc", 1e9); }

// --- Sous-thème : Interphase ---
int DataFile::getInterphaseLabel() const { return get_mat_val<int>(m_config, "interphase", "label", 3); }
double DataFile::getInterphaseE() const     { return get_mat_val<double>(m_config, "interphase", "E", 2000.0); }
double DataFile::getInterphaseNu() const    { return get_mat_val<double>(m_config, "interphase", "nu", 0.38); }
double DataFile::getInterphaseAlpha() const { return get_mat_val<double>(m_config, "interphase", "alpha", 70e-6); }
double DataFile::getInterphaseXt() const    { return get_mat_val<double>(m_config, "interphase", "Xt", 40.0); }
double DataFile::getInterphaseXc() const    { return get_mat_val<double>(m_config, "interphase", "Xc", 100.0); }
double DataFile::getInterphaseG_Ic() const  { return get_mat_val<double>(m_config, "interphase", "GIc", 100.0); }

// ==========================================
// THÈME 4 : [analyses] (Le menu modulaire)
// ==========================================
bool DataFile::runElasticity() const { return get_val<bool>(m_config, "analyses", "calculer_rigidite_elastique", true); }
bool DataFile::runThermal() const    { return get_val<bool>(m_config, "analyses", "calculer_thermique", false); }
bool DataFile::runPDA() const        { return get_val<bool>(m_config, "analyses", "calculer_pda", false); }
string DataFile::getPDALoadCase() const { return get_val<string>(m_config, "analyses", "pda_load_case", "traction_y"); }
