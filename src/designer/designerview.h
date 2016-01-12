#ifndef DESIGNERVIEW_H
#define DESIGNERVIEW_H

#include <QGLViewer/qglviewer.h>
#include "scene.h"

class QMouseEvent;
class Designer;

class DesignerView : public QGLViewer {
    Q_OBJECT
public:
    explicit DesignerView(QWidget *parent = 0);
    
    void setScene(Scene *scene);

    void setDesigner(Designer *designer) {
        this->designer = designer;
    }

    void setMode(ParticleType m) {
        this->mode = m;
    }

    void setSelectedSolidBoundary(int i) {
        selected_boundary = i;
        updateGL();
    }

    void init();
    void draw();

    void paintGrid();

    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void wheelEvent(QWheelEvent *e);
    void mouseReleaseEvent(QMouseEvent *e) {
        down = false;
        QGLViewer::mouseReleaseEvent(e);
    }

    point toWorld(QMouseEvent *e);

    int getSize() const {
        return size;
    }

    void setSize(int size) {
        this->size = size;
        updateGL();
    }

public slots:
    void sceneUpdated();

private:
    void renderPolygon(const polygon &p) const;
    void renderRectangle();
    void renderFluid();
    void renderLine();
    void renderHighlight();

    void drawLines();
    void drawFLuids();
    void drawRects();
    void drawNonGridParticles();

    QPointF getConnectPointOfRect(QRectF r,bool left);
    QRectF isPointInRects(QPointF p);
    QRectF getFluidInBasin(QRectF r, QPointF p);
    QRectF makeRect(QPointF p1, QPointF p2);
    QPointF getRectEdgePointFromMousePoint(QPointF p);


    void drawsphlines(QLineF line);
    void drawsphline(QLineF line);
    void addFluidParticles();
    void addRectangleParticles();
    void addLineParticles();
    bool addPolygonalParticles(polygon *b, ParticleType type);
    bool savePolygon(polygon *b, ParticleType type);
    bool addParticle(const point &around, int size, ParticleType type);

    Scene *scene = 0;
    Designer *designer = 0;
    polygon *current_polygon = 0;
    ParticleType mode = Pan;
    QRectF rectangle;
    QRectF fluid;
    QLineF line;
    QPointF highlightP;
    double cutoffradius = 0.0;



    /*
    std::vector<polygon> fluid1Polygons;
    std::vector<polygon> fluid2Polygons;
    std::vector<polygon> boundaryPolygons;
    */

    bool drawingPolygon;
    bool drawingline = false;
    bool drawingfluid = false;
    bool drawingRectangle = false;

    float boundary_color[3]  = {0.0, 0.0, 0.0};
    float fluid1_color[3]    = {0.0, 0.7, 0.95};
    float fluid2_color[3]    = {0.0, 0.3, 0.95};
    float highlight_color[3] = {0.9, 0.3, 0.3};
    float white[3]           = {1.0, 1.0, 1.0};
    float orange[3]          = {1.0, .62, .47};
    float green[3]           = {.51, 1.0, 0.0};


    point mouse = point{0.0, 0.0};
    int pointsize = 9;

//    int selected_boundary = -1, selected_fluid = -1;
    bool down = false; //mouse down

    int size = 0; //size of rect for del and add

    // the selected polygon
    int selected_boundary = -1, selected_fluid = -1;
};

#endif // DESIGNERVIEW_H
