#ifndef SIMULATION_CONTEXT_H
#define SIMULATION_CONTEXT_H

#include "Mesh.h"
#include "Material.h"
#include "TopologyAnalyzer.h"
#include "Datafile.h"

// Structure "Data-Holder" pour éviter la prolifération des arguments (Parameter Creep)
struct SimulationContext {
    const Mesh& mesh;
    const MaterialManager& mats;
    const TopologyAnalyzer& topo;
    const DataFile& config;
};

#endif // SIMULATION_CONTEXT_H