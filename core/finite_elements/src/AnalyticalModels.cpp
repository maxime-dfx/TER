#include "AnalyticalModels.h"

namespace AnalyticalModels {

    double planeStrainModulus(double E, double nu) {
        return E / (1.0 - nu * nu);
    }

    double computeVoigtE2(double Ef, double nuf, double Em, double num, double Vf) {
        double Vm = 1.0 - Vf;
        double E_f_ps = planeStrainModulus(Ef, nuf);
        double E_m_ps = planeStrainModulus(Em, num);
        return Vf * E_f_ps + Vm * E_m_ps;
    }

    double computeReussE2(double Ef, double nuf, double Em, double num, double Vf) {
        double Vm = 1.0 - Vf;
        double E_f_ps = planeStrainModulus(Ef, nuf);
        double E_m_ps = planeStrainModulus(Em, num);
        return 1.0 / (Vf / E_f_ps + Vm / E_m_ps);
    }

    double computeHalpinTsaiE2(double Ef, double Em, double num, double Vf, double xi) {
        double eta = ((Ef / Em) - 1.0) / ((Ef / Em) + xi);
        double E_HalpinTsai_3D = Em * (1.0 + xi * eta * Vf) / (1.0 - eta * Vf);
        
        // Conversion en déformations planes (Hypothèse de la matrice continue)
        return E_HalpinTsai_3D / (1.0 - num * num);
    }

    double computeSchaperyTransverseAlpha(double Ef, double af, double nuf, 
                                          double Em, double am, double num, double Vf) {
        double Vm = 1.0 - Vf;
        
        // Module longitudinal (Loi des mélanges simple)
        double alpha_L = (Ef * af * Vf + Em * am * Vm) / (Ef * Vf + Em * Vm);
        // Coefficient de Poisson longitudinal
        double nu_L = nuf * Vf + num * Vm;
        
        // Alpha Transverse selon Schapery
        return (1.0 + nuf) * af * Vf + (1.0 + num) * am * Vm - alpha_L * nu_L;
    }

}