#include <iostream>
#include <iomanip>
#include <sys/stat.h>

#include "Mesh.h"
#include "Material.h"
#include "Solver.h"
#include "PostProcessor.h"
#include "Datafile.h" 

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
    string configFile = (argc > 1) ? argv[1] : "simulation.toml";

    try {
        cout << "\n=== SIMULATION TER : ANALYSE MICRO-MECANIQUE ===" << endl;
        
        // 1. Chargement de la configuration via la classe DataFile
        DataFile data(configFile);
        
        cout << ">> Configuration chargee : " << configFile << endl;
        cout << "   -> Mesh : " << data.getMeshFile() << endl;

        ensureDir(data.getOutputDir());

        // 2. Setup
        Mesh mesh;
        mesh.read(data.getMeshFile());
        auto bb = mesh.getBounds();

        MaterialManager mats;
        mats.addMaterial(data.getMatriceLabel(), data.getMatriceE(), data.getMatriceNu());
        mats.addMaterial(data.getFibreLabel(),   data.getFibreE(),   data.getFibreNu());

        Solver solver(mesh, mats);
        double strain = data.getStrainTarget();
        double Lx = bb.L;

        // CL (Pilotage en déplacement)
        solver.addBoundaryCondition([&bb](double x, double y){ return abs(x-bb.xmin)<1e-6; }, 0, 0.0);
        solver.addBoundaryCondition([&bb](double x, double y){ return abs(x-bb.xmax)<1e-6; }, 0, Lx * strain);
        solver.addBoundaryCondition([&bb](double x, double y){ return abs(x-bb.xmin)<1e-6 && abs(y-bb.ymin)<1e-6; }, 1, 0.0);
        
        solver.assemble();
        solver.applyBoundaryConditions();
        solver.solve();

        // 3. Analyse
        PostProcessor post(mesh, mats, solver.getSolution());
        DetailedResults res = post.runAnalysis(bb.L * bb.H, strain, bb.H, data.getFibreLabel(), data.getMatriceLabel());

        // 4. Calcul des bornes et Modèles
        double Vf = res.Vf;
        double Vm = 1.0 - Vf;

        // --- A. Bornes de Voigt & Reuss (En Plane Strain) ---
        auto planeStrainProps = [](double E, double nu) { return E / (1.0 - nu*nu); };
        double E_f_ps = planeStrainProps(data.getFibreE(), data.getFibreNu());
        double E_m_ps = planeStrainProps(data.getMatriceE(), data.getMatriceNu());

        double E_Voigt = Vf * E_f_ps + Vm * E_m_ps;
        double E_Reuss = 1.0 / (Vf/E_f_ps + Vm/E_m_ps);

        // --- B. Estimation de Halpin-Tsai (Ajout demandé) ---
        // Formule standard pour E2 (Transverse)
        double Em = data.getMatriceE();
        double Ef = data.getFibreE();
        double xi = 2.0; // Facteur de forme (2 pour fibres circulaires)
        
        double eta = ((Ef / Em) - 1.0) / ((Ef / Em) + xi);
        double E_HalpinTsai_3D = Em * (1.0 + xi * eta * Vf) / (1.0 - eta * Vf);

        // Conversion approximative pour comparaison (3D -> Plane Strain)
        // On applique le même facteur de rigidification que la matrice
        double E_HalpinTsai_PS = E_HalpinTsai_3D / (1.0 - data.getMatriceNu()*data.getMatriceNu());

        // 5. Affichage
        printLine();
        double toGPa = 1.0/1000.0;
        
        cout << left << setw(20) << "Reuss (Borne Inf)"  << setw(15) << E_Reuss * toGPa << " GPa" << endl;
        // On affiche Halpin-Tsai juste avant le FEM
        cout << left << setw(20) << "Halpin-Tsai (Est.)" << setw(15) << E_HalpinTsai_PS * toGPa << " GPa" << endl;
        cout << left << setw(20) << "FEM (Calcul)"       << setw(15) << res.E_eff * toGPa  << " GPa" << endl;
        cout << left << setw(20) << "Voigt (Borne Sup)"  << setw(15) << E_Voigt * toGPa << " GPa" << endl;
        printLine();

        string fullPath = data.getOutputDir() + "/" + data.getOutputVtk();
        post.exportToVTK(fullPath);

    } catch (const exception& e) {
        cerr << "ERREUR: " << e.what() << endl;
        return 1;
    }
    return 0;
}