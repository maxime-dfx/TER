#ifndef DATAFILE_H
#define DATAFILE_H

#include <string>
#include <iostream>
#include "toml.hpp" 

class DataFile {
public:
    DataFile(const std::string& filename);

    // ==========================================
    // THÈME 1 : [io] (Entrées / Sorties)
    // ==========================================
    std::string getMeshFile() const;
    std::string getOutputDir() const;
    std::string getOutputVtk() const;

    // ==========================================
    // THÈME 2 : [simulation] (Paramètres de calcul)
    // ==========================================
    std::string getLoadCase() const;
    bool        isPlaneStrain() const;
    double      getStrainTarget() const;
    double      getHalpinTsaiXi() const;
    double      getDeltaT() const; 
    int         getLoadSteps() const; 
    bool includeCuringStresses() const;
    double getCuringDeltaT() const;

    // ==========================================
    // THÈME 3 : [materiaux] (Propriétés physiques)
    // ==========================================
    
    // --- Sous-thème : Matrice ---
    int    getMatriceLabel() const;
    double getMatriceE() const;
    double getMatriceNu() const;
    double getMatriceAlpha() const; 
    double getMatriceXt() const;
    double getMatriceXc() const;
    double getMatriceG_Ic() const; 


    // --- Sous-thème : Fibre ---
    int    getFibreLabel() const;
    double getFibreE() const;
    double getFibreNu() const;
    double getFibreAlpha() const; 
    double getFibreXt() const;
    double getFibreXc() const;
    double getFibreG_Ic() const; 

    // --- Sous-thème : Interphase ---
    int    getInterphaseLabel() const;
    double getInterphaseE() const;
    double getInterphaseNu() const;
    double getInterphaseAlpha() const; 
    double getInterphaseXt() const;
    double getInterphaseXc() const;
    double getInterphaseG_Ic() const; 


    bool runElasticity() const;
    bool runThermal() const;
    bool runPDA() const;
    std::string getPDALoadCase() const;
    
private:
    toml::value m_config;
};

#endif