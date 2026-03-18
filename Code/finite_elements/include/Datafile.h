#ifndef DATAFILE_H
#define DATAFILE_H

#include <string>
#include <iostream>
#include "toml.hpp" 

class DataFile {
public:
    DataFile(const std::string& filename);

    // --- IO ---
    std::string getMeshFile() const;
    std::string getOutputDir() const;
    std::string getOutputVtk() const;

    // --- Simulation ---
    double getStrainTarget() const;
    bool   isPlaneStrain() const;
    
    // NOUVELLES METHODES INDISPENSABLES
    std::string getLoadCase() const;
    double getHalpinTsaiXi() const;

    // --- Matériaux : Matrice ---
    int    getMatriceLabel() const;
    double getMatriceE() const;
    double getMatriceNu() const;

    // --- Matériaux : Fibre ---
    int    getFibreLabel() const;
    double getFibreE() const;
    double getFibreNu() const;

private:
    toml::value m_config;
};

#endif