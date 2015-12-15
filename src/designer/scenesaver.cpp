#include "scenesaver.h"
#include "scene.h"
#include <serializer.h>
#include <parser.h>
//#include <3rdparty/qjson/src/serializer.h>
//#include <3rdparty/qjson/src/parser.h>
#include <QVariantList>
#include <QVariantMap>
#include <QFile>
#include <QDebug>
#include <QStringList>
#include <QRectF>

#include <boost/foreach.hpp>

QVariantMap save_parameters(Scene * s) {
    QVariantMap p;
    p["sampling_dist"] = s->getSamplingDistance();
    p["width"] = s->getWidth();
    p["height"] = s->getHeight();
    p["neighbours"] = s->getNeighbours();
    p["c"] = s->getC();
    p["no_slip"] = s->getNoSlip();
    p["alpha"] = s->getAlpha();
    p["epsilon_xsph"] = s->getXSPH();
    p["shepard"] = s->getShepard();
    p["t_damp"] = s->getDampingFactor();
    p["g"] = QVariantList({s->getAccelerationX(), s->getAccelerationY()});
    return p;
}
double snap(double x, double dx) {
    return std::round(x / dx) * dx;
}
QVariantList save_particle_list(const grid &g, double dx, ParticleType type) {
    QVariantList all;

    for (int x = 0; x < g.get_width(); x++) {
        for (int y = 0; y < g.get_height(); y++) {
            if (g(x, y) != type) {
                continue;
            }
            QVariantMap m;
            m["x"] = x*dx;
            m["y"] = y*dx;
            all.append(m);
        }
    }

    return all;
}
QVariantList save_fluid_rects(std::vector<QRectF> fluids){
    QVariantList all;

    BOOST_FOREACH(QRectF &r, fluids) {
        QPointF p1 = r.topLeft();
        QPointF p2 = r.bottomRight();
        QVariantMap m;
        QVariantMap topleft;
        QVariantMap botright;
        topleft["x"] = QString::number(p1.x());
        topleft["y"] = QString::number(p1.y());

        botright["x"] = QString::number(p2.x());
        botright["y"] = QString::number(p2.y());

        m["topleft"] = topleft;
        m["botright"] = botright;
        all.append(m);
    }
    return all;
}

QVariantList save_boundary_rects(std::vector<QRectF> rects){
    QVariantList all;

    BOOST_FOREACH(QRectF &r, rects) {
        QPointF p1 = r.topLeft();
        QPointF p2 = r.bottomRight();
        QVariantMap m;
        QVariantMap topleft;
        QVariantMap botright;
        topleft["x"] = QString::number(p1.x());
        topleft["y"] = QString::number(p1.y());

        botright["x"] = QString::number(p2.x());
        botright["y"] = QString::number(p2.y());

        m["topleft"] = topleft;
        m["botright"] = botright;
        all.append(m);
    }
    return all;
}

QVariantList save_boundary_lines(std::vector<QLineF> lines){
    QVariantList all;

    BOOST_FOREACH(QLineF &l, lines) {
        QPointF p1 = l.p1();
        QPointF p2 = l.p2();
        QVariantMap m;
        QVariantMap VMp1;
        QVariantMap VMp2;
        VMp1["x"] = QString::number(p1.x());
        VMp1["y"] = QString::number(p1.y());

        VMp2["x"] = QString::number(p2.x());
        VMp2["y"] = QString::number(p2.y());

        m["p1"] = VMp1;
        m["p2"] = VMp2;
        all.append(m);
    }
    return all;
}

void setScene(QVariantMap root, Scene *s){
    double width = root["scene"].toMap()["width"].toDouble();
    double height = root["scene"].toMap()["height"].toDouble();
    double samplingDist = root["scene"].toMap()["sampling_dist"].toDouble();
    double neighbours = root["scene"].toMap()["neighbours"].toDouble();
    double alpha = root["scene"].toMap()["alpha"].toDouble();
    double c = root["scene"].toMap()["c"].toDouble();
    double xsph = root["scene"].toMap()["epsilon_xsph"].toDouble();
    double gx = root["scene"].toMap()["g"].toList()[0].toDouble();
    double gy = root["scene"].toMap()["g"].toList()[1].toDouble();
    double noslip = root["scene"].toMap()["no_slip"].toDouble();
    double shepard = root["scene"].toMap()["shepard"].toDouble();
    double damp = root["scene"].toMap()["damp"].toDouble();

    s->setGrid(width,height,samplingDist);
    s->setNeighbours(neighbours);
    s->setAlpha(alpha);
    s->setC(c);
    s->setXSPH(xsph);
    s->setAccelerationX(gx);
    s->setAccelerationY(gy);
    s->setNoSlip(noslip);
    s->setShepard(shepard);
    s->setDampingFactor(damp);

}

void addBoundarys(QVariantMap root, Scene *s){
    QVariantList particles = root["boundary_particles"].toList();
    for(int i = 0; i < particles.size();i++){
        point p{particles.at(i).toMap()["x"].toDouble(),particles.at(i).toMap()["y"].toDouble()};
        s->addParticle(p,Boundary);
    }

}

void addfluids(QVariantMap root, Scene *s){
    QVariantList particles = root["fluid_particles"].toList();
    for(int i = 0; i < particles.size(); i++){
        point p{particles.at(i).toMap()["x"].toDouble(),particles.at(i).toMap()["y"].toDouble()};
        s->addParticle(p,Fluid1);
    }

}
void addFluidRects(QVariantMap root,Scene *s){
    QVariantList fluids = root["fluid_rects"].toList();
    for(int i = 0; i< fluids.size(); i++){
        QVariantMap m = fluids.at(i).toMap();
        QPointF tl = QPointF(m["topleft"].toMap()["x"].toDouble(), m["topleft"].toMap()["y"].toDouble());
        QPointF br = QPointF(m["botright"].toMap()["x"].toDouble(), m["botright"].toMap()["y"].toDouble());
        s->addFluidRect(QRectF(tl,br));
    }
}

void addBoundaryRects(QVariantMap root,Scene *s){
    QVariantList rects = root["boundary_rects"].toList();
    for(int i = 0; i< rects.size(); i++){
        QVariantMap m = rects.at(i).toMap();
        QPointF tl = QPointF(m["topleft"].toMap()["x"].toDouble(), m["topleft"].toMap()["y"].toDouble());
        QPointF br = QPointF(m["botright"].toMap()["x"].toDouble(), m["botright"].toMap()["y"].toDouble());
        s->addBoundaryRect(QRectF(tl,br));
    }
}

void addBoundaryLines(QVariantMap root,Scene *s){
    QVariantList lines = root["boundary_lines"].toList();
    for(int i = 0; i< lines.size(); i++){
        QVariantMap m = lines.at(i).toMap();
        QPointF p1 = QPointF(m["p1"].toMap()["x"].toDouble(), m["p1"].toMap()["y"].toDouble());
        QPointF p2 = QPointF(m["p2"].toMap()["x"].toDouble(), m["p2"].toMap()["y"].toDouble());
        s->addBoundaryLines(QLineF(p1,p2));
    }
}


void save_scene(Scene *s, const QString &file_name) {
    QVariantMap file;

    file["scene"] = save_parameters(s);
    file["fluid_particles"] = save_particle_list(s->const_grid, s->getSamplingDistance(), Fluid1);
    file["boundary_particles"] = save_particle_list(s->const_grid, s->getSamplingDistance(), Boundary);
    file["fluid_rects"] = save_fluid_rects(s->fluid1s);
    file["boundary_rects"] = save_boundary_rects(s->rects);
    file["boundary_lines"] = save_boundary_lines(s->lines);


    QJson::Serializer serializer;
    bool ok;
    QByteArray json = serializer.serialize(file, &ok);
    if(!ok)
        qWarning("Error while creating json");

    QFile f(file_name);
    f.open(QFile::WriteOnly | QFile::Truncate);
    f.write(json);
    f.close();
}

void open_scene(Scene *s, const QString &file_name) {

    QFile f(file_name);
    QJson::Parser parser;
    bool ok;
    f.open(QFile::ReadOnly);

    QByteArray content = f.readAll();
    qDebug() << content;

    QVariantMap root = parser.parse(content, &ok).toMap();
    if (!ok) {
        qDebug() << parser.errorString().toUtf8();
        return;
    }

    setScene(root, s);
    addBoundarys(root,s);
    addfluids(root, s);
    addFluidRects(root,s);
    addBoundaryRects(root,s);
    addBoundaryLines(root,s);

}


std::vector<point> addLineParticles(QLineF l, double samplingDistance){

    // using the bresehnheim algorithm to draw a line between p1 and p2.
    // https://en.wikipedia.org/wiki/Bresenham's_line_algorithm#Method
    std::vector<point> to_add;
    double dx = samplingDistance;
    double dy = samplingDistance;
    QPointF *p1 = new QPointF(snap(l.p1().x(),dx),snap(l.p1().y(),dx));
    QPointF *p2 = new QPointF(snap(l.p2().x(),dx),snap(l.p2().y(),dx));

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
        return to_add;

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

    return to_add;

}

std::vector<point> addRectangleParticles(QRectF rectangle,double sampledist)
{
    std::vector<point> to_add;
    const double dx = sampledist;
    double width = fabs(rectangle.width()/dx);
    double height = fabs(rectangle.height()/dx);


    for(double i = 0; i<= width;i++){
            to_add.push_back(point{rectangle.left()+ i*dx,rectangle.bottom()}); // bot line
    }


    for(double i = 0; i<= height;i++){

            to_add.push_back(point{rectangle.left(),rectangle.bottom()+i*dx}); // left line
            to_add.push_back(point{rectangle.right(),rectangle.bottom()+i*dx}); // right line
    }

    return to_add;

}

std::vector<point> addFluidParticles(QRectF fluid, double sampledistance)
{
    std::vector<point> to_add;
    const double dx = sampledistance;
    double width = fabs(fluid.width()/dx);
    double height = fabs(fluid.height()/dx);


    for(double j = 0; j<=height;j++){
        for(double i = 0; i<= width;i++){
            if(fluid.left() < fluid.right()){
                if( fluid.bottom() < fluid.top()){
                    to_add.push_back(point{fluid.left()+ i*dx,fluid.bottom() + j*dx});
                }else{
                    to_add.push_back(point{fluid.left()+ i*dx,fluid.top() + j*dx});
                }
            }else{
                if( fluid.bottom() < fluid.top()){
                    to_add.push_back(point{fluid.right()+ i*dx,fluid.bottom() + j*dx});
                }else{
                    to_add.push_back(point{fluid.right()+ i*dx,fluid.top() + j*dx});
                }
            }
        }
    }

    return to_add;
}


void export_scene_to_particle_json(Scene *s, const QString &file_name)
{
    // convert all objects to particles in grid
    BOOST_FOREACH(QLineF &l, s->lines) {
        s->addParticles(addLineParticles(l,s->getSamplingDistance()),Boundary);
    }
    BOOST_FOREACH(QRectF &r, s->rects) {
        s->addParticles(addRectangleParticles(r,s->getSamplingDistance()),Boundary);
    }
    BOOST_FOREACH(QRectF &f, s->fluid1s) {
        s->addParticles(addFluidParticles(f,s->getSamplingDistance()),Fluid1);
    }

    // write grid in json
    QVariantMap file;

    file["scene"] = save_parameters(s);
    file["fluid_particles"] = save_particle_list(s->const_grid, s->getSamplingDistance(), Fluid1);
    file["boundary_particles"] = save_particle_list(s->const_grid, s->getSamplingDistance(), Boundary);


    QJson::Serializer serializer;
    bool ok;
    QByteArray json = serializer.serialize(file, &ok);
    if(!ok)
        qWarning("Error while creating json");

    QFile f(file_name);
    f.open(QFile::WriteOnly | QFile::Truncate);
    f.write(json);
    f.close();

    s->clearGrid();
}
