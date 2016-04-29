#include "scene.h"

Scene::Scene() {
}

void Scene::LJSimulationFinished()
{
    // remove particles from current simulation and adds
    // them scene grid
    for(int i = 0; i< this->Sim->x.size();i++){
        this->addParticleToNonGrid(point{this->Sim->x[i],this->Sim->y[i]});
    }
    this->Sim->clear();
}
