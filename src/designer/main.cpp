#include <QApplication>
#include <QGLFormat>
#include "designer.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QGLFormat glf = QGLFormat::defaultFormat();
    glf.setSampleBuffers(true);
    glf.setSamples(8);
    QGLFormat::setDefaultFormat(glf);

    Designer designer;
    designer.show();
    app.connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));
    app.exec();
    return 0;
}
