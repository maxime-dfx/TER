#include <iostream>
#include <iomanip>
#include <sys/stat.h>
#include "Mesh.h"
#include "Material.h"
#include "Solver.h"
#include "PostProcessor.h"

using namespace std;

void ensureDir(const string& path) {
    #if defined(_WIN32)
        mkdir(path.c_str());
    #else 
        mkdir(path.c_str(), 0755);
    #endif
}

void printLine(int width = 60) {
    cout << setfill('-') << setw(width) << "-" << setfill(' ') << endl;
}

int main(int argc, char** argv) {
    string meshFile = (argc > 1) ? argv[1] : "data/maillage/maillage_17216.mesh";
    const int LBL_MATRICE = 101;
    const int LBL_FIBRE   = 102;
    
    // --- NOTE SUR LES MATERIAUX ---
    // E_m=257 GPa est très proche de E_f=311 GPa.
    // Le contraste est faible, donc les bornes Voigt/Reuss seront très serrées.
    double E_m = 257000, nu_m = 0.33; 
    double E_f = 311000, nu_f = 0.19; 
    double strain = 0.001; 

    ensureDir("data/resultats");

    try {
        cout << "\n=== SIMULATION TER : ANALYSE MICRO-MECANIQUE ===" << endl;
        
        Mesh mesh;
        mesh.read(meshFile);
        auto bb = mesh.getBounds();

        MaterialManager mats;
        mats.addMaterial(LBL_MATRICE, E_m, nu_m);
        mats.addMaterial(LBL_FIBRE, E_f, nu_f);

        Solver solver(mesh, mats);
        solver.addBoundaryCondition([&bb](double x, double y){ return abs(x-bb.xmin)<1e-6; }, 0, 0.0);
        solver.addBoundaryCondition([&bb](double x, double y){ return abs(x-bb.xmax)<1e-6; }, 0, bb.L * strain);
        solver.addBoundaryCondition([&bb](double x, double y){ return abs(x-bb.xmin)<1e-6 && abs(y-bb.ymin)<1e-6; }, 1, 0.0);
        
        solver.assemble();
        solver.applyBoundaryConditions();
        solver.solve();

        PostProcessor post(mesh, mats, solver.getSolution());
        DetailedResults res = post.runAnalysis(bb.L * bb.H, strain, bb.H, LBL_FIBRE, LBL_MATRICE);

        // --- CALCUL DES BORNES EN DEFORMATION PLANE (Validité 2D) ---
        double Vf = res.Vf;
        double Vm = 1.0 - Vf;

        // 1. On convertit les propriétés de phase en "Plane Strain Modulus"
        double E_f_ps = E_f / (1.0 - nu_f * nu_f);
        double E_m_ps = E_m / (1.0 - nu_m * nu_m);

        // 2. Calcul des bornes sur ces modules modifiés
        double E_Voigt_PS = Vf * E_f_ps + Vm * E_m_ps;
        double E_Reuss_PS = 1.0 / (Vf/E_f_ps + Vm/E_m_ps);

        // --- AFFICHAGE ---
        cout << "\n";
        printLine();
        cout << " RAPPORT DE VALIDATION (Hypothese Plane Strain)" << endl;
        printLine();
        cout << " Taux de Fibre (Vf) : " << fixed << setprecision(2) << res.Vf*100 << " %" << endl;
        printLine();
        
        cout << " >> COMPARAISON COHERENTE (En GPa)" << endl;
        cout << left << setw(20) << "Modele" << setw(15) << "Module E*" << setw(15) << "Ecart" << endl;
        
        // Affichage en GPa pour lisibilité
        double toGPa = 1.0/1000.0;
        
        cout << left << setw(20) << "Reuss (Borne Inf)" << setw(15) << E_Reuss_PS * toGPa << setw(15) << (E_Reuss_PS-res.E_eff)/res.E_eff*100 << " %" << endl;
        cout << left << setw(20) << "FEM (Calcul)"      << setw(15) << res.E_eff * toGPa  << setw(15) << "-" << endl;
        cout << left << setw(20) << "Voigt (Borne Sup)" << setw(15) << E_Voigt_PS * toGPa << setw(15) << (E_Voigt_PS-res.E_eff)/res.E_eff*100 << " %" << endl;
        printLine();

        cout << " Note : E* est le module apparent en deformation plane." << endl;
        cout << " Si FEM est entre les bornes, le calcul est valide." << endl;
        printLine();

        post.exportToVTK("data/resultats/traction_resultat.vtk");
        post.exportToGnuplot("data/resultats/data_field.dat");

    } catch (const exception& e) {
        cerr << "ERREUR: " << e.what() << endl;
        return 1;
    }
    return 0;
}