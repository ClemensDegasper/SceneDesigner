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
    void recLoop(std::vector<QRectF> rects, std::vector<QRectF> OriginalRects, std::vector<QLineF> OriginalLines, QList<basin> basins,std::vector<QRectF> fluids, int ptr);
    int PointAtTopLeftOfRect(QPointF p, std::vector<QRectF> OriginalRects);
    int PointAtTopRightOfRect(QPointF p, std::vector<QRectF> OriginalRects);
    QLineF adjustLineToNewRects(std::vector<QRectF> rects,std::vector<QRectF> OriginalRects, QLineF l);
    QRectF adjustFluidsToNewRects(std::vector<QRectF> rects, std::vector<QRectF> OriginalRects,QRectF f);

    QLineEdit* LayoutItemToLineEdit(QLayoutItem *qli);
    int BasinCounter;
    int SampleSceneCounter=0;
    Scene *s;
};

#endif // SCENESAMPLER_H
