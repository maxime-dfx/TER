#include "AnalyticalModels.h"
#include <cmath>

namespace AnalyticalModels {

    double computeVoigtE1(double Ef, double Em, double Vf) {
        return Vf * Ef + (1.0 - Vf) * Em;
    }

    double computeReussE2(double Ef, double Em, double Vf) {
        return 1.0 / (Vf / Ef + (1.0 - Vf) / Em);
    }

    double computeHalpinTsaiE2(double Ef, double Em, double Vf, double xi) {
        double eta = ((Ef / Em) - 1.0) / ((Ef / Em) + xi);
        return Em * (1.0 + xi * eta * Vf) / (1.0 - eta * Vf);
    }

    double computeChamisE2(double Ef, double Em, double Vf) {
        return Em / (1.0 - std::sqrt(Vf) * (1.0 - Em / Ef));
    }

    double computeMoriTanakaG12(double Gf, double Gm, double Vf) {
        double Vm = 1.0 - Vf;
        return Gm + (Vf * Gm * (Gf - Gm)) / (Gm + 0.5 * Vm * (Gf - Gm));
    }

    double computeSchaperyTransverseAlpha(double Ef, double af, double nuf, 
                                          double Em, double am, double num, double Vf) {
        double Vm = 1.0 - Vf;
        double alpha_L = (Ef * af * Vf + Em * am * Vm) / (Ef * Vf + Em * Vm);
        double nu_L = nuf * Vf + num * Vm;
        
        return (1.0 + nuf) * af * Vf + (1.0 + num) * am * Vm - alpha_L * nu_L;
    }

}