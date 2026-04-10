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
    std::string getLoadCase() const;
    double getHalpinTsaiXi() const;
    double getDeltaT() const; 

    // --- Matériaux : Matrice ---
    int    getMatriceLabel() const;
    double getMatriceE() const;
    double getMatriceNu() const;
    double getMatriceAlpha() const; 

    // --- Matériaux : Fibre ---
    int    getFibreLabel() const;
    double getFibreE() const;
    double getFibreNu() const;
    double getFibreAlpha() const; 

private:
    toml::value m_config;
};

#endif