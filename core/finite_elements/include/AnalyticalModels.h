#ifndef ANALYTICAL_MODELS_H
#define ANALYTICAL_MODELS_H

namespace AnalyticalModels {

    double computeVoigtE1(double Ef, double Em, double Vf);
    double computeReussE2(double Ef, double Em, double Vf);
    double computeHalpinTsaiE2(double Ef, double Em, double Vf, double xi = 2.0);
    double computeChamisE2(double Ef, double Em, double Vf);
    
    double computeMoriTanakaG12(double Gf, double Gm, double Vf);

    double computeSchaperyTransverseAlpha(double Ef, double af, double nuf, 
                                          double Em, double am, double num, double Vf);

}

#endif // ANALYTICAL_MODELS_H