#ifndef SCENESAMPLER_H
#define SCENESAMPLER_H

#include <QDialog>
#include "scene.h"
#include "QRectF"
#include "QLayoutItem"
#include "QLineEdit"


struct basin{
    double xadd;
    double xstep;
    double yadd;
    double ystep;
};

namespace Ui {
class SceneSampler;
}

class SceneSampler : public QDialog
{
    Q_OBJECT

public:
    explicit SceneSampler(QWidget *parent = 0, Scene *s = 0);
    ~SceneSampler();

private slots:
    void on_buttonBox_accepted();

private:
    Ui::SceneSampler *ui;
    void addBasinToGui(QRectF r, int BasinCounter, int RowCounter);
    QLineEdit* LayoutItemToLineEdit(QLayoutItem *qli);
    int BasinCounter;
    Scene *s;
};

#endif // SCENESAMPLER_H
