#include "designerview.h"
#include "scene.h"
#include "designer.h"

#include <QMouseEvent>
#include <QDebug>
#include <cmath>
#include <boost/foreach.hpp>

//CGAL Stuff
#include <CGAL/bounding_box.h>
#include <CGAL/squared_distance_2.h>
#include <CGAL/Cartesian.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Polygon_2_algorithms.h>
#include <CGAL/Bbox_2.h>
#include <CGAL/Point_set_2.h>
#include <CGAL/Triangulation_vertex_base_with_info_2.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/range_search_delaunay_2.h>

typedef CGAL::Cartesian<double>                 K;
//typedef K::Line_2                               Line;
typedef K::Point_2                              Point;
typedef CGAL::Polygon_2<K>                      Polygon;
typedef CGAL::Bbox_2                            Bbox;
typedef K::Segment_2                            Segment;
typedef K::Iso_rectangle_2                      Iso_rectangle;
typedef CGAL::Point_set_2<K>::Vertex_handle     Vertex_handle;

inline
double snap(double x, double dx) {
    return std::round(x / dx) * dx;
}

inline
point snap_point(double x, double y, double dx) {
    return point{snap(x, dx), snap(y, dx)};
}


DesignerView::DesignerView(QWidget *parent) :
    QGLViewer(parent) {

    setMouseBinding(Qt::LeftButton, CAMERA, TRANSLATE);
    setMouseBinding(Qt::NoButton, CAMERA, ROTATE);
    setMouseBinding(Qt::NoButton, FRAME, ROTATE);
}

void DesignerView::setScene(Scene *scene) {
    this->scene = scene;
    using namespace qglviewer;
    setSceneBoundingBox(Vec(0, 0, 0), Vec(scene->getWidth(), scene->getHeight(), 0));
    showEntireScene();
    connect(scene, SIGNAL(changed()), SLOT(sceneUpdated()));
    sceneUpdated();
}

void DesignerView::init() {
    this->camera()->setType(qglviewer::Camera::ORTHOGRAPHIC);
    glClearColor(.8, .8, .8, 1.0);
    setMouseTracking(true);

    drawingPolygon = false;
}

void DesignerView::draw() {
    glClear(GL_COLOR_BUFFER_BIT);

    //render snapped mouse position
    glColor3fv(white);
    drawText(40, 40, QString("%1, %2").arg(mouse.x).arg(mouse.y));

    //render scene borders
    glLineWidth(3);
    glColor3f(1.0, 0.0, 0.0);
    glBegin(GL_LINE_LOOP);
        glVertex2f(0, 0);
        glVertex2f(scene->getWidth(), 0);

        glVertex2f(scene->getWidth(), scene->getHeight());
        glVertex2f(0, scene->getHeight());
        //glVertex2f(scene->getWidth(), 0);
    glEnd();

    if (drawingPolygon) {
        renderPolygon(*current_polygon);
    }
    if(drawingRectangle) {
        renderRectangle();
    }
    if(drawingfluid) {
        renderFluid();
    }
    if(drawingline){
        renderLine();
    }

    glPointSize(this->pointsize);
    glBegin(GL_POINTS);
    double dx = scene->getSamplingDistance();

    for (int x = 0; x < scene->const_grid.get_width(); x++) {
        for (int y = 0; y < scene->const_grid.get_height(); y++) {
            if (scene->const_grid(x, y) == None) {
                continue;
            }
            if (scene->const_grid(x, y) == ParticleType::Fluid1) {
                glColor3fv(fluid1_color);
            }
            if (scene->const_grid(x, y) == ParticleType::Fluid2) {
                glColor3fv(fluid2_color);
            }
            if (scene->const_grid(x, y) == ParticleType::Boundary) {
                glColor3fv(boundary_color);
            }
            glVertex2f(x*dx, y*dx);
        }
    }
    glEnd();

    //render grid
    paintGrid();
}

void DesignerView::wheelEvent(QWheelEvent *e){

    double step = e->delta()/8;
    if(step < 0){

        this->pointsize++;
        qDebug()<< "zoom in";
    }else{
        if(this->pointsize > 1)
            this->pointsize--;
        qDebug()<< "zomm out";
    }

    QGLViewer::wheelEvent(e);
}

void DesignerView::mousePressEvent(QMouseEvent *e) {
    down = true;

    if (mode == Pan) {
        QGLViewer::mousePressEvent(e);
        return;
    }
    /*
    if (e->button() == Qt::RightButton) {
        if (drawingPolygon) {
            if (mode == (Boundary || Fluid1 || Fluid2)) {
                current_polygon->points.push_back(current_polygon->points.at(0));
                savePolygon(current_polygon, mode);
                addPolygonalParticles(current_polygon, mode);
            }
            delete current_polygon;
            drawingPolygon = false;
            return;
        } else {
            if (mode == Fluid1 || mode == Fluid2 || mode == Boundary || mode == None ) {
                addParticle(mouse, size, mode);
            }
        }
    }

    if ((mode == Fluid1 || mode == Fluid2 || mode == Boundary)
            && e->button() == Qt::LeftButton
            && drawingPolygon) {
        current_polygon->points.push_back(mouse);
        updateGL();
        return;
    }


    if ((mode == Fluid1 || mode == Fluid2 || mode == Boundary) && e->button() == Qt::LeftButton && !drawingPolygon) {
        drawingPolygon = true;
        current_polygon = new polygon;
        current_polygon->points.push_back(mouse);
        updateGL();
        return;
    }
    */

    if(mode == Line && e->button() == Qt::LeftButton){
        if(drawingline){
            line->setP2(QPointF(mouse.v[0], mouse.v[1]));
            addLineParticles();
        }else{
            line = new QLineF(QPointF(mouse.v[0], mouse.v[1]),QPointF(mouse.v[0], mouse.v[1]));
        }
        drawingline = !drawingline;
        updateGL();
    }

    if(mode == Fluid1 && e->button() == Qt::LeftButton){
        if(drawingfluid){
            fluid->setBottomRight(QPointF(mouse.v[0], mouse.v[1]));
            addFluidParticles();
        }else{
            fluid = new QRectF(QPointF(mouse.v[0],mouse.v[1]),QPointF(mouse.v[0],mouse.v[1]));
        }
        drawingfluid = !drawingfluid;
        updateGL();
    }

    if(mode == Rectangle && e->button() == Qt::LeftButton) {
        if(drawingRectangle){
            rectangle->setBottomRight(QPointF(mouse.v[0],mouse.v[1]));
            addRectangleParticles();
        }else{
            rectangle = new QRectF(QPointF(mouse.v[0],mouse.v[1]),QPointF(mouse.v[0],mouse.v[1]));
        }
        drawingRectangle = !drawingRectangle;
        updateGL();
    }

}

void DesignerView::mouseMoveEvent(QMouseEvent *e) {
    mouse = toWorld(e);
    updateGL();

    if (mode == Pan) {
        QGLViewer::mouseMoveEvent(e);
        return;
    }

    if (drawingPolygon)  {
        current_polygon->last() = toWorld(e);
    }
    if(drawingRectangle) {
        rectangle->setBottomRight(QPointF(toWorld(e).v[0],toWorld(e).v[1]));
    }
    if(drawingfluid){
        fluid->setBottomRight(QPointF(toWorld(e).v[0],toWorld(e).v[1]));
    }
    if(drawingline){

        line->setP2(QPointF(toWorld(e).v[0],toWorld(e).v[1]));
    }
    if(down) {
        if(mode == Boundary)    //continously drawing boundrys
            addParticle(mouse, size, mode);
        if(mode == None)        //continously deleting particles
            addParticle(mouse, 2, mode);
    }
}

point DesignerView::toWorld(QMouseEvent *e) {
    makeCurrent();
    qglviewer::Vec screen(e->posF().x(), e->posF().y(), 0.0);
    qglviewer::Vec world = camera()->unprojectedCoordinatesOf(screen);

    double dx = scene->getSamplingDistance();

    return snap_point(world.x, world.y, dx);
}

void DesignerView::sceneUpdated() {    
    updateGL();
}

void DesignerView::renderPolygon(const polygon &b) const {
    glLineWidth(3.0);

    if (b.first() == b.last()) {
        glBegin(GL_LINE_LOOP);
            BOOST_FOREACH(const point &p, b.points) {
                glVertex2dv(p.v);
            }
        glEnd();
    } else {
        glBegin(GL_LINE_STRIP);
            BOOST_FOREACH(const point &p, b.points) {
                glVertex2dv(p.v);
            }
        glEnd();
    }
}

void DesignerView::renderRectangle()
{

    glLineWidth(3.0);
    glBegin(GL_LINE_LOOP);
    glVertex2d(rectangle->topLeft().x(),rectangle->topLeft().y());
    glVertex2d(rectangle->bottomLeft().x(),rectangle->bottomLeft().y());
    glVertex2d(rectangle->bottomRight().x(),rectangle->bottomRight().y());
    glVertex2d(rectangle->topRight().x(),rectangle->topRight().y());
    glEnd();

}

void DesignerView::renderFluid()
{
    glColor3fv(fluid1_color);
    glLineWidth(3.0);
    glBegin(GL_LINE_LOOP);
    glVertex2d(fluid->topLeft().x(),fluid->topLeft().y());
    glVertex2d(fluid->bottomLeft().x(),fluid->bottomLeft().y());
    glVertex2d(fluid->bottomRight().x(),fluid->bottomRight().y());
    glVertex2d(fluid->topRight().x(),fluid->topRight().y());
    glEnd();
}

void DesignerView::renderLine()
{
    glLineWidth(3.0);
    glBegin(GL_LINE_STRIP);
    glVertex2d(line->p1().x(),line->p1().y());
    glVertex2d(line->p2().x(),line->p2().y());
    glEnd();
}

void DesignerView::addFluidParticles()
{
    std::vector<point> to_add;
    const double dx = scene->getSamplingDistance();
    double width = fabs(fluid->width()/dx);
    double height = fabs(fluid->height()/dx);


    for(double j = 0; j<=height;j++){
        for(double i = 0; i<= width;i++){
            if(fluid->left() < fluid->right()){
                if( fluid->bottom() < fluid->top()){
                    to_add.push_back(point{fluid->left()+ i*dx,fluid->bottom() + j*dx});
                }else{
                    to_add.push_back(point{fluid->left()+ i*dx,fluid->top() + j*dx});
                }
            }else{
                if( fluid->bottom() < fluid->top()){
                    to_add.push_back(point{fluid->right()+ i*dx,fluid->bottom() + j*dx});
                }else{
                    to_add.push_back(point{fluid->right()+ i*dx,fluid->top() + j*dx});
                }
            }
        }
    }

    scene->addParticles(to_add, Fluid1);
}

void DesignerView::addRectangleParticles()
{
    std::vector<point> to_add;
    const double dx = scene->getSamplingDistance();
    bool first = true;
    double width = fabs(rectangle->width()/dx);
    double height = fabs(rectangle->height()/dx);


    for(double i = 0; i<= width;i+=dx){
        if(rectangle->left() < rectangle->right()){
            to_add.push_back(point{rectangle->left()+ i*dx,rectangle->top()}); // top line
            to_add.push_back(point{rectangle->left()+ i*dx,rectangle->bottom()}); // bot line
        }else{
            to_add.push_back(point{rectangle->right()+ i*dx,rectangle->top()}); // top line
            to_add.push_back(point{rectangle->right()+ i*dx,rectangle->bottom()}); // bot line
        }
    }


    for(double i = 0; i< height;i+=dx){
        if(first){
            first = false;
            continue;
        }
        if(rectangle->bottom() > rectangle->top()){
            to_add.push_back(point{rectangle->left(),rectangle->top()+i*dx}); // left line
            to_add.push_back(point{rectangle->right(),rectangle->top()+i*dx}); // right line
        }else{
            to_add.push_back(point{rectangle->left(),rectangle->bottom()+i*dx}); // left line
            to_add.push_back(point{rectangle->right(),rectangle->bottom()+i*dx}); // right line
        }
    }


    scene->addParticles(to_add, Boundary);

}

void DesignerView::addLineParticles(){
    std::vector<point> to_add;
    const double dx = scene->getSamplingDistance();
    QPointF *p1 = new QPointF(snap(line->p1().x(),dx),snap(line->p1().y(),dx));
    QPointF *p2 = new QPointF(snap(line->p2().x(),dx),snap(line->p2().y(),dx));

    double deltaX = p2->x() - p1->x();
    double deltaY = p2->y() - p1->y();

    double error = 0;
    double deltaError = fabs(deltaY/deltaX);

    double startx,endx,starty,endy;
    if(p1->x() < p2->x()){
        startx = p1->x();
        endx = p2->x();
    }else{
        startx = p2->x();
        endx = p1->x();
    }
    if(p1->y() < p2->y()){
        starty = p1->y();
        endy = p2->y();
    }else{
        starty = p2->y();
        endy = p1->y();
    }

    for(double x = startx; x < endx; x += dx){

        to_add.push_back(point{x,starty});
        error += deltaError;
        if(error >= 0.5){
            starty += 1;
            error -= 1;
        }
    }
    scene->addParticles(to_add,Boundary);

}


bool DesignerView::savePolygon(polygon *b, ParticleType type) {
    switch (type) {
    case Boundary:
        boundaryPolygons.push_back(*b);
        qDebug() << "adding boundary";
        renderPolygon(*b);
        updateGL();
        break;
    case Fluid1:
        fluid1Polygons.push_back(*b);
        qDebug() << "adding fluid1";
        renderPolygon(*b);
        updateGL();
        break;
    case Fluid2:
        fluid2Polygons.push_back(*b);
        qDebug() << "adding fluid2";
        renderPolygon(*b);
        updateGL();
        break;
    default:
        break;
    }
}

bool DesignerView::addPolygonalParticles(polygon *b, ParticleType type) {
    if (b->points.size() < 2) return true;

    Polygon pgn;

    if(b->first() == b->last()) {
        b->points.pop_back();
    }

    BOOST_FOREACH(const point &p, b->points) {
        pgn.push_back(Point(p.x, p.y));
    }

    if (!pgn.is_simple())
        return false;

    assert(!pgn.is_empty());

    Bbox bbox = pgn.bbox();

    const double dx = scene->getSamplingDistance();

    point min = snap_point(bbox.xmin(), bbox.ymin(), dx);
    point max = snap_point(bbox.xmax(), bbox.ymax(), dx);

    std::vector<point> to_add;

    for (double x = min.x; x < max.x; x += dx) {
        for (double y = min.y; y < max.y; y += dx) {
            Point p(x, y);
            if (pgn.bounded_side(p) == CGAL::ON_BOUNDED_SIDE) {
                to_add.push_back(point{x, y});
            }
        }
    }

    scene->addParticles(to_add, type);

    return true;
}

bool DesignerView::addParticle(const point &around, int size, ParticleType type) {
    double dx = scene->getSamplingDistance();
    std::vector<point> to_add;
    for (int x = -size; x <= size; x++) {
        for (int y = -size; y <= size; y++) {
            to_add.push_back(snap_point(around.x + dx*x, around.y + dx*y, dx));
        }
    }
    scene->addParticles(to_add, type);
}

void DesignerView::paintGrid() {
    Q_ASSERT(scene);
    glColor3f(0.81, 0.81, 0.0);
    glLineWidth(1.0);
    glBegin(GL_LINES);
    double width = scene->getWidth();
    double height = scene->getHeight();
    double dx = scene->getSamplingDistance();

    for (double x = -2*dx; x <= width + 2*dx; x += dx) {
        glVertex2f(x, -2*dx);
        glVertex2f(x,  height + 2*dx);
    }

    for (double y = -2*dx; y <= height + 2*dx; y += dx) {
        glVertex2f(-2*dx, y);
        glVertex2f(width + 2*dx, y);
    }

    glEnd();
}
