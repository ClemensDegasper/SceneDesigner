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


void save_scene(Scene *s, const QString &file_name) {
    QVariantMap file;

    file["fluid_particles"] = save_particle_list(s->const_grid, s->getSamplingDistance(), Fluid1);
    file["boundary_particles"] = save_particle_list(s->const_grid, s->getSamplingDistance(), Boundary);

    file["scene"] = save_parameters(s);

    QJson::Serializer serializer;
    bool ok;

    QByteArray json = serializer.serialize(file, &ok);

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


}




























