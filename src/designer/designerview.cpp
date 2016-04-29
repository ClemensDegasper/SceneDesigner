#include "designerview.h"
#include "scene.h"
#include "designer.h"

#include <QMouseEvent>
#include <QDebug>
#include <QApplication>
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

    //loadTexture("/home/clemens/Pictures/designer.png",inFlow);

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
    //renderInFlow();

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
    if(drawingRepairCircle){
        renderRepairCircle();
    }
    if(drawingRepairSquare){
        renderRepairSquare();
    }

    if(!this->highlightP.isNull()){
        renderHighlight();
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
    drawNonGridParticles();
    drawPolygons();
    if(mode == RepairPoly)
        drawSimParticles();



    //render grid
    paintGrid();
    renderFlows();

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

void DesignerView::drawNonGridParticles()
{
    // draw from non grid
    glPointSize(this->pointsize);
    glBegin(GL_POINTS);
    glColor3fv(boundary_color);
    BOOST_FOREACH(const point &p, this->scene->nongrid) {
        glVertex2d(p.x,p.y);
    }
    glEnd();
}

void DesignerView::drawSimParticles()
{
    // draw lennard jones particles
    glPointSize(this->pointsize);
    glBegin(GL_POINTS);
    glColor3fv(boundary_color);
    for(int i = 0; i<this->scene->Sim->N;i++){
        glVertex2d(this->scene->Sim->x[i],this->scene->Sim->y[i]);
    }
    glEnd();

}

void DesignerView::renderRepairCircle()
{
    glColor3fv(orange);
    glBegin(GL_LINE_LOOP);
    for(double i = 0; i < 2 * M_PI; i += M_PI / 12)
        glVertex2d(circleRadius.p1().x() + cos(i) * circleRadius.length(),circleRadius.p1().y() + sin(i) * circleRadius.length());
    glEnd();
}

void DesignerView::renderRepairSquare()
{
//    double distance = sqrt(pow(RepairLeft.x() - RepairRight.x(),2) + pow(RepairLeft.y() - RepairRight.y(),2));
//    QLineF line = QLineF(RepairLeft,RepairRight);
//    QLineF norm = line.normalVector();
//    norm.setLength(distance);
//    QPointF move = QPointF(fabs(norm.p1().x()-norm.p2().x()),fabs(norm.p1().y()-norm.p2().y()));
    glLineWidth(3.0);
    glBegin(GL_LINE_LOOP);
    glColor3fv(orange);
    glVertex2d(RepairRect.topLeft().x(),RepairRect.topLeft().y());
    glVertex2d(RepairRect.bottomLeft().x(),RepairRect.bottomLeft().y());
    glVertex2d(RepairRect.bottomRight().x(),RepairRect.bottomRight().y());
    glVertex2d(RepairRect.topRight().x(),RepairRect.topRight().y());
    glEnd();

}

void DesignerView::addingNewLine(QLineF l)
{
    // check if line is connected to topright edge of basin and then adjust it
    // + cutoff radius because wall is that big
    QPointF tmp = QPointF(this->scene->getCutOffRadius()-this->scene->getSamplingDistance(),0);
    BOOST_FOREACH(const QRectF &r, this->scene->rects){
        if(l.p1().rx() == r.topRight().rx() && l.p1().ry() == r.topRight().ry()){
            l.setP1(l.p1()+tmp);
            break;
        }
        if(l.p2().rx() == r.topRight().rx() && l.p2().ry() == r.topRight().ry()){
            l.setP2(l.p2()+tmp);
            break;
        }
        if(l.p1().rx() == r.topLeft().rx() && l.p1().ry() == r.topLeft().ry()){
            l.setP1(l.p1()-tmp);
            break;
        }
        if(l.p2().rx() == r.topLeft().rx() && l.p2().ry() == r.topLeft().ry()){
            l.setP2(l.p2()-tmp);
            break;
        }
    }
    qDebug()<<l.angle();
    this->scene->lines.push_back(l);
}

void DesignerView::fillRepairRect()
{
    int savecounter = 0;
    double xmin,xmax,ymin,ymax;
    double dx = this->scene->getSamplingDistance();
    if(RepairRect.left() < RepairRect.right()){
        xmin = RepairRect.left();
        xmax = RepairRect.right();
    }else{
        xmin = RepairRect.right();
        xmax = RepairRect.left();
    }
    if(RepairRect.bottom() < RepairRect.top()){
        ymin = RepairRect.bottom();
        ymax = RepairRect.top();
    }else{
        ymin = RepairRect.top();
        ymax = RepairRect.bottom();
    }
    for(double x = xmin; x <= xmax; x+=dx){
        for(double y = ymin; y <= ymax; y+=dx){
            this->scene->nongrid.push_back(point{snap(x,dx),snap(y,dx)});
            savecounter++;
            if(savecounter > 1000)
                return;
        }
    }
}

QImage DesignerView::loadTexture(char *filename, GLuint &textureID)
{
    glEnable(GL_TEXTURE_2D); // Enable texturing
    glGenTextures(1, &textureID); // Obtain an id for the texture
    glBindTexture(GL_TEXTURE_2D, textureID); // Set as the current texture
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

    QImage im(filename);
    QImage tex = QGLWidget::convertToGLFormat(im);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, tex.bits());

    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

    glDisable(GL_TEXTURE_2D);
    return tex;
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

QPointF DesignerView::getRectEdgePointFromMousePoint(QPointF mouse)
{
    QRectF r = isPointInRects(mouse);
    QPointF highlightPoint;
    if(!r.isNull()){
        if(mouse.x() < r.left()){
            highlightPoint = r.topLeft();
        }else{
            if(mouse.x() > r.right()){
                highlightPoint = r.topRight();
            }else{
                if(mouse.x() < r.center().x()){
                    highlightPoint = r.topLeft();
                }else{
                    highlightPoint = r.topRight();
                }
            }
        }
    }else{
        highlightPoint = QPointF(0,0);
    }
    return highlightPoint;
}

void DesignerView::drawsphlines(QLineF l)
{

    double dx = this->scene->getSamplingDistance();
    double startx, starty, endx, endy, distance, step,x,y;

    QPointF *p1 = new QPointF(l.p1().x(),l.p1().y());
    QPointF *p2 = new QPointF(l.p2().x(),l.p2().y());
    QLineF norm = l.normalVector();                     //get normal vector from line
    QLineF unitvec = norm.fromPolar(dx,norm.angle());   // get create vector with sample distance length and angle from normal vector
    QLineF perpendicularLine = l;
    for(int i = 1; i < this->scene->getCutOffRadius()*100; i++){
        perpendicularLine.setP1(QPointF(perpendicularLine.x1() + unitvec.x2(),perpendicularLine.y1()+unitvec.y2()));        // add unitvec to line to get perpedicular line
        perpendicularLine.setP2(QPointF(perpendicularLine.x2()+unitvec.x2(),perpendicularLine.y2()+unitvec.y2()));

        drawsphline(perpendicularLine);
    }
    startx = p1->x();
    starty = p1->y();
    endx = p2->x();
    endy = p2->y();

    distance = sqrt(pow(endx - startx,2)+pow(endy-starty,2));


    step = (this->scene->getSamplingDistance() / distance);
    for(double i = 0; i <= 1; i+=step){
        x = startx + (endx - startx) * i;
        y = starty + (endy - starty) * i;
        this->scene->addParticleToNonGrid(point{x,y});
    }
}
void DesignerView::drawsphline(QLineF l)
{

    double dx = this->scene->getSamplingDistance();
    double startx, starty, endx, endy, distance, step,x,y;

    QPointF *p1 = new QPointF(l.p1().x(),l.p1().y());
    QPointF *p2 = new QPointF(l.p2().x(),l.p2().y());

    startx = p1->x();
    starty = p1->y();
    endx = p2->x();
    endy = p2->y();

    distance = sqrt(pow(endx - startx,2)+pow(endy-starty,2));


    step = (this->scene->getSamplingDistance() / distance);
    for(double i = 0; i <= 1; i+=step){
        x = startx + (endx - startx) * i;
        y = starty + (endy - starty) * i;
        this->scene->addParticleToNonGrid(point{x,y});
    }
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

//        qDebug() << this->scene->fluid1s.size();
//        qDebug() << this->scene->rects.size();
//        qDebug() << this->scene->lines.size();

//        BOOST_FOREACH(const QRectF &r, this->scene->rects) {
//            qDebug() << r.topLeft().x();
//            qDebug() << r.topLeft().y();
//            qDebug() << r.bottomRight().x();
//            qDebug() << r.bottomRight().y();
//        }
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


    if (mode == RepairPoly && e->button() == Qt::RightButton && QApplication::keyboardModifiers() == Qt::ControlModifier) {
        // adding last point to polygon
        // setting last point also as first point to close it
        // adding polygon to poly vector
        if (drawingPolygon) {
            current_polygon->points.push_back(current_polygon->points.at(0));
            this->scene->polys.push_back(*current_polygon);
            drawingPolygon = false;


            // using cgal polygon for boundry test
            polygon poly = this->scene->polys.at(0);             // our "normal" polygon
            Polygon Pgn;                            // cgal polygon

            if(poly.first() == poly.last()) {
                poly.points.pop_back();
            }

            BOOST_FOREACH(const point &b, poly.points) {
                Pgn.push_back(Point(b.x, b.y));         // fill cgal poly from our own points
            }


            for(int i = this->scene->nongrid.size()-1; i >= 0; i--){
                point p = this->scene->nongrid.at(i);   //our "normal" point
                Point cp(p.v[0],p.v[1]);                // cgal point

                // erase particle if they are inside polygon
                if(Pgn.bounded_side(cp) == CGAL::ON_BOUNDED_SIDE){
                    this->scene->nongrid.erase(this->scene->nongrid.begin()+i);
                }
            }
            // draw outlines of polygon

            //line from last point to first
            point p1 = poly.points.at(poly.points.size()-1);
            point p2 = poly.points.at(0);
            qDebug()<<p1.v[0] << p1.v[1];
            QLineF l = QLineF(p1.v[0],p1.v[1],p2.v[0],p2.v[1]);
            this->scene->Sim->addline(l,this->scene->getSamplingDistance());

            //all other lines
            for(int i = 1; i<poly.points.size();i++){
                point p1 = poly.points.at(i-1);
                point p2 = poly.points.at(i);
                QLineF l = QLineF(p1.v[0],p1.v[1],p2.v[0],p2.v[1]);
                this->scene->Sim->addline(l,this->scene->getSamplingDistance());
            }
            this->scene->polys.clear();
            return;
        }
    }




    // adding point to poly
    if ((mode == RepairPoly) && e->button() == Qt::LeftButton && drawingPolygon && QApplication::keyboardModifiers() == Qt::ControlModifier) {
        current_polygon->points.push_back(mouse);
        updateGL();
        return;
    }


    // first click of poly
    if ((mode == RepairPoly) && e->button() == Qt::LeftButton && !drawingPolygon && QApplication::keyboardModifiers() == Qt::ControlModifier) {
        drawingPolygon = true;
        current_polygon = new polygon;
        current_polygon->points.push_back(mouse);
        current_polygon->points.push_back(mouse); // adding two points because mouse move event doesnt add point but alters last one
        updateGL();
        return;
    }

    if(mode == RepairPoly && e->button() == Qt::LeftButton && !drawingPolygon){
        this->scene->Sim->addParticle(realMouse.v[0],realMouse.v[1]);
        return;
    }

    if(mode == RepairCircle && e->button() == Qt::LeftButton){
        if(drawingRepairCircle){
            int counter = 0;
            // adds all up in list, remove from nongrid
            for(int i = this->scene->nongrid.size()-1; i >= 0; i--){
                point p = this->scene->nongrid.at(i);
                if(isParticleInCircle(p,this->circleRadius)){
                    counter ++;
                    this->scene->nongrid.erase(this->scene->nongrid.begin()+i);
                }
            }
            distributeParticles(500,this->circleRadius);
        }else{
            circleRadius = QLineF(QPointF(mouse.v[0],mouse.v[1]),QPointF(mouse.v[0],mouse.v[1]));
        }
        drawingRepairCircle = !drawingRepairCircle;
    }

    if(mode == RepairSquare && e->button() == Qt::LeftButton){
        if(drawingRepairSquare){
            // remove all particles in drawn repairsquare from nongrid
            for(int i = this->scene->nongrid.size()-1; i >= 0; i--){
                point p = this->scene->nongrid.at(i);
                if(RepairRect.contains(QPointF(p.x,p.y))){
                    this->scene->nongrid.erase(this->scene->nongrid.begin()+i);
                }
            }
            // add new better ones
            fillRepairRect();
        }else{
            RepairRect = QRectF(QPointF(realMouse.v[0],realMouse.v[1]),QPointF(realMouse.v[0],realMouse.v[1]));
        }
        drawingRepairSquare= !drawingRepairSquare;
    }

    if(mode == Line && e->button() == Qt::LeftButton){
        if(drawingline){
            // Check if end line is in rects
            // if yes connect line directly to right edge of rect
            QPointF endpoint = QPointF(mouse.v[0], mouse.v[1]);

            // check if second click is in any basins
            // if yes put point at according edge of basin
            QPointF tmp = getRectEdgePointFromMousePoint(endpoint);
            if(!tmp.isNull())
                endpoint = tmp;
            //else set mousepoint as p2 of line
            line.setP2(endpoint);
            //addLineParticles();
            addingNewLine(line);
            //this->scene->lines.push_back(line);//line->p1().x(),line->p1().y(),line->p2().x(),line->p2().y()));
            //drawsphlines(line);

        }else{
            line = QLineF(QPointF(mouse.v[0], mouse.v[1]),QPointF(mouse.v[0], mouse.v[1]));
            QPointF startpoint = QPointF(mouse.v[0],mouse.v[1]);

            // first click behave same as second
            QPointF tmp = getRectEdgePointFromMousePoint(startpoint);
            if(!tmp.isNull())
                startpoint = tmp;
            line.setP1(startpoint);
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
    /*
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
    */
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


    if(mode == PlaceInFlow){
        this->pInFlow = point{e->x(),e->y()};//mouse.v[0],mouse.v[1]};
    }

    if(mode == PlaceOutFlow){
        this->pOutFlow = point{e->x(),e->y()};//mouse.v[0],mouse.v[1]};
    }
}

void DesignerView::mouseMoveEvent(QMouseEvent *e) {
    mouse = toWorld(e);
    realMouse = toWorld(e,false);
    updateGL();

    if(mode == RepairPoly && QApplication::keyboardModifiers() == Qt::ControlModifier){
        if(drawingPolygon){
            current_polygon->last() = realMouse;
            return;
        }

    }
    if (mode == Pan) {
        QGLViewer::mouseMoveEvent(e);
        return;
    }

    if(mode == RepairCircle){
        if(drawingRepairCircle){
            circleRadius.setP2(QPointF(mouse.v[0],mouse.v[1]));
            qDebug()<<circleRadius.length();
        }
    }

    if(mode == RepairSquare){
        if(drawingRepairSquare){
            RepairRect.setBottomLeft(QPointF(realMouse.v[0],realMouse.v[1]));
        }
    }

    if(mode == Line){
        if(drawingline){
            line.setP2(QPointF(toWorld(e).v[0],toWorld(e).v[1]));
        }
        // set hightlight point for p2
        QPointF mousepoint = QPointF(mouse.v[0],mouse.v[1]);
        this->highlightP = getRectEdgePointFromMousePoint(mousepoint);

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

    if(down) {
        if(mode == Boundary)    //continously drawing boundrys
            addParticle(mouse, size, mode);
        if(mode == None)        //continously deleting particles
            addParticle(mouse, 2, mode);
    }
}

void DesignerView::keyPressEvent(QKeyEvent *e)
{
    if(mode == RepairPoly && e->key() == Qt::Key_S){
        this->scene->Sim->takeStep();
        updateGL();
        return;
    }
    if(mode == RepairPoly && e->key() == Qt::Key_I){
        this->scene->Sim->computeAccelerations();
        updateGL();
        return;
    }
}

point DesignerView::toWorld(QMouseEvent *e) {
    makeCurrent();
    qglviewer::Vec screen(e->posF().x(), e->posF().y(), 0.0);
    qglviewer::Vec world = camera()->unprojectedCoordinatesOf(screen);

    double dx = scene->getSamplingDistance();

    return snap_point(world.x, world.y, dx);
}
point DesignerView::toWorld(QMouseEvent *e, bool useSnap){
    makeCurrent();
    qglviewer::Vec screen(e->posF().x(), e->posF().y(), 0.0);
    qglviewer::Vec world = camera()->unprojectedCoordinatesOf(screen);

    double dx = scene->getSamplingDistance();
    if(useSnap)
        return snap_point(world.x, world.y, dx);
    else
        return point{world.x,world.y};
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

void DesignerView::renderHighlight()
{
    glPointSize(5);
    glBegin(GL_POINTS);
    glColor3fv(green);
    glVertex2f(this->highlightP.x(),this->highlightP.y());
    glEnd();
}

void DesignerView::renderInFlow()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, inFlow);

    glBegin(GL_QUADS);

        glTexCoord2f(0,0); glVertex2f(0, 0);
        glTexCoord2f(0.05,0); glVertex2f(0.05, 0);
        glTexCoord2f(0.05,0.05); glVertex2f(0.05, 0.05);
        glTexCoord2f(0,0.05); glVertex2f(0, 0.05);

    glEnd();

    glDisable(GL_TEXTURE_2D);
}

void DesignerView::renderFlows()
{
    glColor3f(0,0,0);
    drawText(this->pOutFlow.v[0], this->pOutFlow.v[1], QString("OutFlow"));
    drawText(this->pInFlow.v[0], this->pInFlow.v[1], QString("InFlow"));
}

void DesignerView::drawPolygons()
{
    glLineWidth(3.0);
    BOOST_FOREACH(polygon &p, this->scene->polys) {
        glColor3fv(orange);
        glBegin(GL_LINE_LOOP);
            BOOST_FOREACH(const point &pt, p.points) {
                glVertex2dv(pt.v);
            }
        glEnd();
        /*
        glBegin(GL_LINE_STRIP);
            BOOST_FOREACH(const point &p, b.points) {
                glVertex2dv(p.v);
            }
        glEnd();
        */
    }
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

void DesignerView::distributeParticles(int n, QLineF circle)
{
    //distributing the particle via sunflower head algorithm
    //http://stackoverflow.com/questions/28567166/uniformly-distribute-x-points-inside-a-circle

    double alpha = 0.1;
    double b = round(alpha*sqrt(n));      //% number of boundary points
    double phi = (sqrt(5)+1)/2;           //% golden ratio
    for(int i = 1; i < n; i++){
            double r = calcRadiusForRepair(i,n,b);
            double theta = 2*M_PI*i/pow(phi,2);
            this->scene->nongrid.push_back(point{circle.p1().x() + r * cos(theta),circle.p1().y() + r * sin(theta)});
    }


}

bool DesignerView::savePolygon(polygon *b, ParticleType type) {
    switch (type) {
    case Fluid2:
        this->scene->polys.push_back(*b);
        qDebug() << "adding polygon";
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

bool DesignerView::isParticleInCircle(point p, QLineF circle)
{
    double dist = pow((circle.p1().x() - p.x),2) + pow((circle.p1().y() - p.y),2);
    return dist <= pow(circle.length(),2);
}

double DesignerView::calcRadiusForRepair(int i, int n, double b)
{
    double r;
    if(i>n-b){
        r = 1;            // put on the boundary
    }else{
        r = sqrt(i-1/2)/sqrt(n-(b+1)/2);     // apply square root
    }

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
