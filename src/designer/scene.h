#ifndef SCENE_H
#define SCENE_H

#include <QObject>
#include <QDebug>
#include <vector>
#include <cassert>
#include <boost/foreach.hpp>
#include <QRectF>
#include <QLineF>

union point {
    struct {
        double x, y;
    };
    double v[2];
    bool operator==(const point &other) const {
        return x == other.x && y == other.y;
    }
};

/**
 * @brief The solid_boundary struct
 */
struct polygon {
    std::vector<point> points;
    point &last() {
        return *(points.end()-1);
    }

    point &first() {
        return *points.begin();
    }

    const point &last() const {
        return *(points.end()-1);
    }

    const point &first() const {
        return *points.begin();
    }
};

enum ParticleType {
    None = 0,
    Fluid1 = 1,
    Fluid2 = 2,
    Boundary = 3,
    Pan = 4,
    Rectangle = 5,
    Line = 6
};

struct grid {
    grid(int width, int height) {
        particles = new ParticleType[width*height];
        resize(width, height);
        clear();
    }

    virtual ~grid() {
        delete[] particles;
    }

    void resize(int new_width, int new_height) {
        ParticleType *new_particles = new ParticleType[new_width * new_height];
#pragma omp parallel for
        for (int i = 0; i < new_width * new_height; i++) {
            new_particles[i] = None;
        }
#pragma omp parallel for
        for (int x = 0; x < std::min(width, new_width); x++) {
            for (int y = 0; y < std::min(height, new_height); y++) {
                new_particles[y*new_width+x] = particles[y*width+x];
            }
        }

        delete[] particles;

        particles = new_particles;

        width = new_width;
        height = new_height;
    }

    ParticleType &operator()(int x, int y) {
        assert(x >= 0 && x < width);
        assert(y >= 0 && y < height);
        return particles[y*width+x];
    }

    const ParticleType &operator()(int x, int y) const {
        assert(x >= 0 && x < width);
        assert(y >= 0 && y < height);
        return particles[y*width+x];
    }

    int get_width() const {
        return width;
    }

    int get_height() const {
        return height;
    }

    void clear() {
        for (int i = 0; i < width*height; ++i) {
            particles[i] = None;
        }
    }

private:
    int width, height;
    ParticleType *particles;

};

class Scene : public QObject {
    Q_OBJECT
public:
    Scene();
    double getSamplingDistance() const { return samplingDistance; }
    double getCutOffRadius() const { return cutoffradius; }
    double getWidth() const { return width; }
    double getHeight() const { return height; }
    double getC() const { return c; }
    double getAlpha() const { return alpha; }
    int getNeighbours() const { return neighbours; }
    double getXSPH() const { return xsph; }
    double getNoSlip() const { return noSlip; }
    double getShepard() const { return shepard; }
    double getAccelerationX() const { return accelerationX; }
    double getAccelerationY() const { return accelerationY; }
    double getDampingFactor() const { return dampingFactor; }


    void addParticleToNonGrid(point p){
        this->nongrid.push_back(p);
    }

    void addParticlesToNonGrid(const std::vector<point> &points) {
        BOOST_FOREACH(const point &p, points) {
            nongrid.push_back(p);
        }
    }

    void setGrid(double width, double height, double samplingDistance) {
        this->width = width;
        this->height = height;
        this->samplingDistance = samplingDistance;
        resize_grid();
        emit changed();
    }

    void setCutoffRadius(double r){
        this->cutoffradius = r;
    }

    void setAccelerationX(double accelerationX) {
        this->accelerationX = accelerationX;
    }

    void setAccelerationY(double accelerationY) {
        this->accelerationY = accelerationY;
    }

    void setShepard(double shepard) {
        this->shepard = shepard;
    }

    void setXSPH(double xsph) {
        this->xsph = xsph;
    }

    void setAlpha(double alpha) {
        this->alpha = alpha;
    }

    void setNeighbours(double neighbours) {
        this->neighbours = neighbours;
    }

    void setC(double c) {
        this->c = c;
    }

    void setDampingFactor(double dampingFactor) {
        this->dampingFactor = dampingFactor;
    }

    void setNoSlip(double noSlip) {
        this->noSlip = noSlip;
    }

    void addSolidBoundary(const polygon &l) {
        //solidBoundaries.push_back(l);
        emit changed();
    }

    void addParticles(const std::vector<point> &points, ParticleType type) {
        BOOST_FOREACH(const point &p, points) {
            int x = snap(p.x);
            int y = snap(p.y);
            int currentCell = g(x,y);
            if(currentCell == None){
                g(x, y) = type;
            }else if(currentCell == Boundary){
                if(type == Boundary or type == Fluid1){
                    continue;
                }
            }else if (currentCell == Fluid1){
                if(type == Boundary){
                    g(x,y) = type;
                }
            }

        }
    }

    void addParticle(const point p, ParticleType type) {
        g(snap(p.x), snap(p.y)) = type;
    }

    void addFluidRect(QRectF r){
        this->fluid1s.push_back(r);
    }

    void addBoundaryRect(QRectF r){
        this->rects.push_back(r);
    }
    void addBoundaryLines(QLineF l){
        this->lines.push_back(l);
    }


    void deleteParticle(const point &p) {
        g(snap(p.x), snap(p.y)) = None;
        emit changed();
    }

    const grid &const_grid = g;
    std::vector<point> nongrid;
    std::vector<QRectF> rects;
    std::vector<QRectF> fluid1s;
    std::vector<QLineF> lines;

    void clearFluids(){
        while(!fluid1s.empty()){
            fluid1s.pop_back();
        }

    }

    void clearLines(){
        while(!lines.empty()){
            lines.pop_back();
        }
    }

    void clearRects(){
        while(!rects.empty()){
            rects.pop_back();
        }
    }
    void clearGrid(){
        g.clear();
        emit changed();
    }

    void clear() {

        clearFluids();
        clearLines();
        clearRects();
        g.clear();
        emit changed();
    }

signals:
    void changed();

private:
    int snap(double x) {
        return std::round(x/samplingDistance);
    }

    void resize_grid() {
        int new_width = std::ceil(width/samplingDistance);
        int new_height = std::ceil(height/samplingDistance);
        g.resize(new_width, new_height);
    }

    double samplingDistance = 0.01;
    double cutoffradius = 0.03;
    double width = 1.0, height = 1.0;
    double accelerationX = 0.0, accelerationY = 9.81;
    int neighbours = 3;
    double xsph = 0.0;
    double dampingFactor = 0.0;
    double shepard = 0.0;
    double noSlip = 0.0;
    int c = 0;
    double alpha = 0.0;



/*
    std::vector<polygon> solidBoundaries;
    std::vector<polygon> fluids;
    std::vector<point> fluid_particles, boundary_particles;
*/
    std::vector<std::vector<ParticleType> > particles;

    grid g = grid(std::ceil(width/samplingDistance), std::ceil(height/samplingDistance));
};

#endif // SCENE_H
