#ifndef ANALYTICAL_MODELS_H
#define ANALYTICAL_MODELS_H

namespace AnalyticalModels {

    // Propriétés en déformations planes
    double planeStrainModulus(double E, double nu);

    // Bornes de Voigt (Supérieure) et Reuss (Inférieure) pour E2 en déformations planes
    double computeVoigtE2(double Ef, double nuf, double Em, double num, double Vf);
    double computeReussE2(double Ef, double nuf, double Em, double num, double Vf);

    // Modèle semi-empirique de Halpin-Tsai pour E2 (ramené en déformations planes)
    double computeHalpinTsaiE2(double Ef, double Em, double num, double Vf, double xi);

    // Loi des mélanges de Schapery pour le coefficient de dilatation thermique (Alpha) transverse
    double computeSchaperyTransverseAlpha(double Ef, double af, double nuf, 
                                          double Em, double am, double num, double Vf);

}

#endif // ANALYTICAL_MODELS_H