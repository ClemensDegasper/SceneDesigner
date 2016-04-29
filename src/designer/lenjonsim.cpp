#include "lenjonsim.h"
#include <cmath>
#include <cstdlib>
#include <QDebug>

LenJonSim::LenJonSim()
{


}

void LenJonSim::computeAccelerations()
{
    for (int i = nonMovableParticles; i < N; i++)
        ax[i] = ay[i] = 0;

    for (int i = N; i >= nonMovableParticles; i--)
    for (int j = i - 1; j > 0; j--) {
        double dx = x[i] - x[j];
        double dy = y[i] - y[j];

        double dr = sqrt(dx * dx + dy * dy);
        dr*=repulsionDistance;
        double f = 3 * pow(dr, -3.25) - 1.5 * pow(dr, -1.75);       // decreased constants to adjust for smaller coordinates
        ax[i] += f * dx / dr;
        ay[i] += f * dy / dr;
        if(j< nonMovableParticles)
            continue;
        ax[j] -= f * dx / dr;
        ay[j] -= f * dy / dr;
    }


}

void LenJonSim::initialize() {

    computeAccelerations();
}

void LenJonSim::timeStep() {

    t += dt;
    for (int i = nonMovableParticles; i < N; i++) {

        // integrate using velocity Verlet algorithm
        x[i] += vx[i] * dt + 0.5 * ax[i] * dt * dt;
        y[i] += vy[i] * dt + 0.5 * ay[i] * dt * dt;

        // periodic boundary conditions
//        if (x[i] < 0) x[i] += L;
//        if (x[i] > L) x[i] -= L;
//        if (y[i] < 0) y[i] += L;
//        if (y[i] > L) y[i] -= L;
        vx[i] += 0.5 * ax[i] * dt;
        vy[i] += 0.5 * ay[i] * dt;

        computeAccelerations();

        vx[i] += 0.5 * ax[i] * dt;
        vy[i] += 0.5 * ay[i] * dt;


    }


}


// call takestep to advance simulation
void LenJonSim::takeStep() {
    timeStep();
}

void LenJonSim::addParticle(double x, double y)
{
    N++;
    this->ax.push_back(0);
    this->ay.push_back(0);
    this->x.push_back(x);
    this->y.push_back(y);
    double pi = 4 * atan(1.0);
    double v = sqrt(2 * kT);
    double theta = 2 * pi * rand() / double(RAND_MAX);
    vx.push_back(0);//v * cos(theta));
    vy.push_back(0);//v * sin(theta));
}

void LenJonSim::printAllParticle()
{
    for(int i = 0; i<N; i++){
        qDebug() << x[i] << "/" << y[i];
    }
}


void LenJonSim::addline(QLineF l,double dx)
{

    double startx, starty, endx, endy, distance, step,x,y;

    QPointF *p1 = new QPointF(l.p1().x(),l.p1().y());
    qDebug()<<p1->x()<<p1->y();
    QPointF *p2 = new QPointF(l.p2().x(),l.p2().y());

    startx = p1->x();
    starty = p1->y();
    endx = p2->x();
    endy = p2->y();

    distance = sqrt(pow(endx - startx,2)+pow(endy-starty,2));


    step = (dx / distance);
    for(double i = 0; i <= 1; i+=step){
        x = startx + (endx - startx) * i;
        y = starty + (endy - starty) * i;
        addParticle(x,y);
        nonMovableParticles++;
    }
}

void LenJonSim::clear()
{
    x.clear();
    y.clear();
    vx.clear();
    vy.clear();
    ax.clear();
    ay.clear();
    N = 0;
    nonMovableParticles = 0;
}



