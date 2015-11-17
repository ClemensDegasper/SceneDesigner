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

    // draw from grid
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

    //draw from objects
    drawLines();
    drawRects();
    drawFLuids();

    //render grid
    paintGrid();
}

void DesignerView::drawRects(){
    glLineWidth(3.0);

    BOOST_FOREACH(const QRectF &r, this->scene->rects) {
        glBegin(GL_LINE_STRIP);
        glColor3fv(boundary_color);

        // check if rects is drawn upside down
        if(r.top() > r.bottom()){
            glVertex2d(r.topLeft().x(),r.topLeft().y());
            glVertex2d(r.bottomLeft().x(),r.bottomLeft().y());
            glVertex2d(r.bottomRight().x(),r.bottomRight().y());
            glVertex2d(r.topRight().x(),r.topRight().y());
        }else{  // switch top and bot
            glVertex2d(r.bottomLeft().x(),r.bottomLeft().y());
            glVertex2d(r.topLeft().x(),r.topLeft().y());
            glVertex2d(r.topRight().x(),r.topRight().y());
            glVertex2d(r.bottomRight().x(),r.bottomRight().y());
        }


        glEnd();
    }
}

QPointF DesignerView::getConnectPointOfRect(QRectF r,bool left)
{
    // if left true, return top left of rect else return top right of rect
    if(left)
    {
        if(r.left() < r.right()){// left is left
            if(r.bottom() < r.top())//bot is bot
            {
                return r.topLeft();
            }else{ //top is bot
                return r.bottomLeft();
            }

        }else{//right is left
            if(r.bottom() < r.top())//bot is bot
            {
                return r.topRight();
            }else{// top is bot
                return r.bottomRight();
            }
        }
    }else{
        if(r.left() < r.right()){// left is left
            if(r.bottom() < r.top())//bot is bot
            {
                return r.topRight();
            }else{ //top is bot
                return r.bottomRight();
            }

        }else{//right is left
            if(r.bottom() < r.top())//bot is bot
            {
                return r.topLeft();
            }else{// top is bot
                return r.bottomLeft();
            }
        }
    }
}

QRectF DesignerView::isPointInRects(QPointF p)
{
    BOOST_FOREACH(const QRectF &r, this->scene->rects) {
        if(r.contains(p))
        {
            return r;
        }
    }
    return QRectF(0,0,0,0);
}

QRectF DesignerView::getFluidInBasin(QRectF r, QPointF p)
{
    QRectF(QPointF(r.left(),p.y()),QPointF(r.right(),r.bottom()));
}

QRectF DesignerView::makeRect(QPointF p1, QPointF p2)
{
    QRectF r = QRectF();
    if(p1.x() < p2.x()){ // p1 is left
        if(p1.y()> p2.y()){ // p1 is top
            r.setTopLeft(p1);
            r.setBottomRight(p2);
        }else{ // p2 is top
            r.setBottomLeft(p1);
            r.setTopRight(p2);
        }
    }else{ // p2 is left
        if(p1.y()> p2.y()){ // p1 is top
            r.setTopRight(p1);
            r.setBottomLeft(p2);
        }else{ // p2 is top
            r.setBottomRight(p1);
            r.setTopLeft(p2);
        }
    }
    return r;
}


void DesignerView::drawFLuids(){
    glLineWidth(3.0);

    BOOST_FOREACH(const QRectF &r, this->scene->fluid1s) {
        glBegin(GL_QUADS);
        glColor3fv(fluid1_color);
        glVertex2d(r.topLeft().x(),r.topLeft().y());
        glVertex2d(r.topRight().x(),r.topRight().y());
        glVertex2d(r.bottomRight().x(),r.bottomRight().y());
        glVertex2d(r.bottomLeft().x(),r.bottomLeft().y());
        glEnd();
    }
}


void DesignerView::drawLines(){
    glLineWidth(3.0);

    BOOST_FOREACH(const QLineF &l, this->scene->lines) {
        glBegin(GL_LINE_STRIP);
        glColor3fv(boundary_color);
        glVertex2d(l.p1().x(),l.p1().y());
        glVertex2d(l.p2().x(),l.p2().y());
        glEnd();
    }

}


void DesignerView::wheelEvent(QWheelEvent *e){

    /*
    double step = e->delta()/8;
    if(step < 0){
        this->pointsize++;
    }else{
        if(this->pointsize > 1)
            this->pointsize--;
    }
    */
    QGLViewer::wheelEvent(e);
}

void DesignerView::mousePressEvent(QMouseEvent *e) {
    down = true;

    if (mode == Pan) {

        qDebug() << this->scene->fluid1s.size();
        qDebug() << this->scene->rects.size();
        qDebug() << this->scene->lines.size();

        BOOST_FOREACH(const QRectF &r, this->scene->rects) {
            qDebug() << r.topLeft().x();
            qDebug() << r.topLeft().y();
            qDebug() << r.bottomRight().x();
            qDebug() << r.bottomRight().y();
        }
        QGLViewer::mousePressEvent(e);
        return;
    }

    if (mode == None){
        //check if deleting fluids
        for(int i = 0; i < this->scene->fluid1s.size(); i++) {
            if(this->scene->fluid1s.at(i).contains(mouse.v[0],mouse.v[1])){
                this->scene->fluid1s.erase(this->scene->fluid1s.begin()+i);
            }
        }

        //check if deleting rects
        for(int i = 0; i < this->scene->rects.size();i++){
            if(this->scene->rects.at(i).contains(mouse.v[0],mouse.v[1])){
                this->scene->rects.erase(this->scene->rects.begin()+i);
            }
        }

        //check if deleting lines
        double epsilon = 0.03;
        QLineF l1 = QLineF(mouse.v[0] - epsilon ,mouse.v[1] - epsilon ,mouse.v[0] + epsilon ,mouse.v[1] + epsilon);
        QLineF l2 = QLineF(mouse.v[0] + epsilon ,mouse.v[1] + epsilon ,mouse.v[0] - epsilon ,mouse.v[1] - epsilon);
        // creating to lines out ot that one mouse coord one for every diagonal

        for(int i = 0; i < this->scene->lines.size();i++){
            if(this->scene->lines.at(i).intersect(l1,new QPointF()) == QLineF::BoundedIntersection || this->scene->lines.at(i).intersect(l2,new QPointF()) == QLineF::BoundedIntersection){   // in the qpoint the exapt point of intersection would be saved
                this->scene->lines.erase(this->scene->lines.begin()+i);
            }
        }
        updateGL();
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
            // Check if end line is in rects
            // if yes connect line directly to right edge of rect
            QPointF endpoint = QPointF(mouse.v[0], mouse.v[1]);
            QRectF r = isPointInRects(endpoint);
            if(!r.isNull()){
                if(line.p1().x() < r.left())
                    endpoint = r.topLeft();//getConnectPointOfRect(r,true);
                else
                    endpoint = r.topRight();getConnectPointOfRect(r,false);
            }

            line.setP2(endpoint);
            //addLineParticles();
            this->scene->lines.push_back(line);//line->p1().x(),line->p1().y(),line->p2().x(),line->p2().y()));

        }else{
            line = QLineF(QPointF(mouse.v[0], mouse.v[1]),QPointF(mouse.v[0], mouse.v[1]));
        }
        drawingline = !drawingline;
        updateGL();
        return;
    }

    if(mode == Fluid1 && e->button() == Qt::LeftButton){
        if(drawingfluid){
            fluid.setBottomRight(QPointF(mouse.v[0], mouse.v[1]));
            this->scene->fluid1s.push_back(fluid);
            //addFluidParticles();
        }else{
            QPointF p = QPointF(mouse.v[0],mouse.v[1]);
            //check if click inside basin
            QRectF r = isPointInRects(p);
            if(!r.isNull()){    // click is in basin, fill basin with fluid
                fluid = QRectF(QPointF(r.left(),p.y()),QPointF(r.right(),r.bottom())); //getFluidInBasin(r, p);
                this->scene->fluid1s.push_back(fluid);
                updateGL();
                return;
            }else{  // click not in basin, make normal fluid
                fluid = QRectF(QPointF(mouse.v[0],mouse.v[1]),QPointF(mouse.v[0],mouse.v[1]));
            }

        }
        drawingfluid = !drawingfluid;
        updateGL();
        return;
    }
    if(mode == Fluid2 && e->button() == Qt::LeftButton){
        if(drawingfluid){
            fluid.setBottomRight(QPointF(mouse.v[0], mouse.v[1]));
            addFluidParticles();
        }else{
            fluid = QRectF(QPointF(mouse.v[0],mouse.v[1]),QPointF(mouse.v[0],mouse.v[1]));
        }
        drawingfluid = !drawingfluid;
        updateGL();
        return;
    }

    if(mode == Rectangle && e->button() == Qt::LeftButton) {
        if(drawingRectangle){
            QPointF firstClick = rectangle.topLeft();
            QPointF secondClick = QPointF(mouse.v[0],mouse.v[1]);
            rectangle = makeRect(firstClick,secondClick);
            this->scene->rects.push_back(QRectF(rectangle));
            //addRectangleParticles();
        }else{
            rectangle = QRectF(QPointF(mouse.v[0],mouse.v[1]),QPointF(mouse.v[0],mouse.v[1]));
        }
        drawingRectangle = !drawingRectangle;
        updateGL();
        return;
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
        rectangle.setBottomRight(QPointF(toWorld(e).v[0],toWorld(e).v[1]));
    }
    if(drawingfluid){
        fluid.setBottomRight(QPointF(toWorld(e).v[0],toWorld(e).v[1]));
    }
    if(drawingline){

        line.setP2(QPointF(toWorld(e).v[0],toWorld(e).v[1]));
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
    glVertex2d(rectangle.topLeft().x(),rectangle.topLeft().y());
    glVertex2d(rectangle.bottomLeft().x(),rectangle.bottomLeft().y());
    glVertex2d(rectangle.bottomRight().x(),rectangle.bottomRight().y());
    glVertex2d(rectangle.topRight().x(),rectangle.topRight().y());
    glEnd();

}

void DesignerView::renderFluid()
{
    glColor3fv(fluid1_color);
    glLineWidth(3.0);
    glBegin(GL_LINE_LOOP);
    glVertex2d(fluid.topLeft().x(),fluid.topLeft().y());
    glVertex2d(fluid.bottomLeft().x(),fluid.bottomLeft().y());
    glVertex2d(fluid.bottomRight().x(),fluid.bottomRight().y());
    glVertex2d(fluid.topRight().x(),fluid.topRight().y());
    glEnd();
}

void DesignerView::renderLine()
{
    glLineWidth(3.0);
    glBegin(GL_LINE_STRIP);
    glVertex2d(line.p1().x(),line.p1().y());
    glVertex2d(line.p2().x(),line.p2().y());
    glEnd();
}


void DesignerView::addFluidParticles()
{
//    std::vector<point> to_add;
//    const double dx = scene->getSamplingDistance();
//    double width = fabs(fluid->width()/dx);
//    double height = fabs(fluid->height()/dx);


//    for(double j = 0; j<=height;j++){
//        for(double i = 0; i<= width;i++){
//            if(fluid->left() < fluid->right()){
//                if( fluid->bottom() < fluid->top()){
//                    to_add.push_back(point{fluid->left()+ i*dx,fluid->bottom() + j*dx});
//                }else{
//                    to_add.push_back(point{fluid->left()+ i*dx,fluid->top() + j*dx});
//                }
//            }else{
//                if( fluid->bottom() < fluid->top()){
//                    to_add.push_back(point{fluid->right()+ i*dx,fluid->bottom() + j*dx});
//                }else{
//                    to_add.push_back(point{fluid->right()+ i*dx,fluid->top() + j*dx});
//                }
//            }
//        }
//    }

//    scene->addParticles(to_add, Fluid1);
}

void DesignerView::addRectangleParticles()
{
    std::vector<point> to_add;
    const double dx = scene->getSamplingDistance();
    bool first = true;
    double width = fabs(rectangle.width()/dx);
    double height = fabs(rectangle.height()/dx);


    for(double i = 0; i<= width;i++){
        if(rectangle.left() < rectangle.right()){
            to_add.push_back(point{rectangle.left()+ i*dx,rectangle.top()}); // top line
            to_add.push_back(point{rectangle.left()+ i*dx,rectangle.bottom()}); // bot line
        }else{
            to_add.push_back(point{rectangle.right()+ i*dx,rectangle.top()}); // top line
            to_add.push_back(point{rectangle.right()+ i*dx,rectangle.bottom()}); // bot line
        }
    }


    for(double i = 0; i< height;i++){
        if(first){
            first = false;
            continue;
        }
        if(rectangle.bottom() > rectangle.top()){
            to_add.push_back(point{rectangle.left(),rectangle.top()+i*dx}); // left line
            to_add.push_back(point{rectangle.right(),rectangle.top()+i*dx}); // right line
        }else{
            to_add.push_back(point{rectangle.left(),rectangle.bottom()+i*dx}); // left line
            to_add.push_back(point{rectangle.right(),rectangle.bottom()+i*dx}); // right line
        }
    }

    scene->addParticles(to_add, Boundary);

}

void DesignerView::addLineParticles(){

    // using the bresehnheim algorithm to draw a line between p1 and p2.
    // https://en.wikipedia.org/wiki/Bresenham's_line_algorithm#Method
    std::vector<point> to_add;
    double dx = scene->getSamplingDistance();
    double dy = scene->getSamplingDistance();
    QPointF *p1 = new QPointF(snap(line.p1().x(),dx),snap(line.p1().y(),dx));
    QPointF *p2 = new QPointF(snap(line.p2().x(),dx),snap(line.p2().y(),dx));

    double deltaX = p2->x() - p1->x();
    double deltaY = p2->y() - p1->y();

    double error = 0;
    double deltaError = fabs(deltaY/deltaX);
    double startx,endx,starty, endy;
    starty = p1->y();
    startx = p1->x();
    endx = p2->x();
    endy = p2->y();
    bool done = false;

    // vertical line special case
    if(startx == endx){
        double y = starty;
        while(!done){
            to_add.push_back(point{startx,y});
            if(starty < endy){
                y += dy;
                if(y >= endy)
                    done = true;
            }else{
                y -= dy;
                if(y <= endy)
                    done =true;
            }
        }
        scene->addParticles(to_add,Boundary);
        return;
    }


    // gernerall cases
    double x = startx;
    while(!done){

        to_add.push_back(point{x,starty});
        error += deltaError;
        while(error >= 0.5){
            to_add.push_back(point{x,starty});
            if(deltaY < 0)
                starty -= dy;
            else
                starty += dy;
            error -= 1;
        }

        // count x up or down depending on end and start points x's
        if(startx < endx){
            x += dx;
            if(x > endx)
                done = true;
        }else{
            x -= dx;
            if(x < endx)
                done =true;
        }
    }

    scene->addParticles(to_add,Boundary);

}

/*
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
}*/

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
