#ifndef LENJONSIM_H
#define LENJONSIM_H
#include <vector>
#include <cmath>
#include <qline.h>


class LenJonSim
{
public:
    LenJonSim();
    int N = 0;                      // number of molecules
    int nonMovableParticles = 0;
    double L = 6;                    // linear size of square region
    double kT = 0.0001;                   // initial kinetic energy/molecule
    int skip = 0;                    // steps to skip to speed graphics

    std::vector<double> x, y, vx, vy;        // position and velocity components
    std::vector<double> ax, ay;              // acceleration components

    double t = 0;                   // time
    double dt = 0.01;               // integration time step
    int step = 0;                   // step number
    int step0;                      // starting step for computing average

    double repulsionDistance = 70;

    int nBins = 50;                 // number of velocity bins
    std::vector<double> vBins;              // for Maxwell-Boltzmann distribution
    double vMax = 4;                // maximum velocity to bin
    double dv = vMax / nBins;       // bin size






    void computeAccelerations();
    void initialize();
    void timeStep();
    void takeStep();
    void addParticle(double x, double y);
    void printAllParticle();
    void addline(QLineF l, double dx);
    void clear();

};




#endif // LENJONSIM_H
