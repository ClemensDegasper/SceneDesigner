#include "scenesampler.h"
#include "ui_scenesampler.h"
#include "qpushbutton.h"
#include "qlabel.h"
#include "QLineEdit"
#include "scenesaver.h"

SceneSampler::SceneSampler(QWidget *parent, Scene *s) :
    QDialog(parent),
    ui(new Ui::SceneSampler)
{
    ui->setupUi(this);
    this->s = s;
    if(this->s->rects.size() == 0){
        QLabel *lbl = new QLabel(this);
        lbl->setText("No Basins found in Scene.");
        ui->grid1->addWidget(lbl);
        lbl->show();

    }
    BasinCounter = 0;
    int RowCounter = 0;
    BOOST_FOREACH(const QRectF &r, this->s->rects) {
        addBasinToGui(r,BasinCounter+1,RowCounter);
        BasinCounter++;
        RowCounter += 3;
    }


}

SceneSampler::~SceneSampler()
{
    delete ui;
}

void SceneSampler::addBasinToGui(QRectF r, int BasinCounter, int RowCounter)
{
    // width and height of basin
    QLabel *basinInfo = new QLabel(this);
    basinInfo->setText(QString("Basin " + QString::number(BasinCounter) + "\n\tHeight: ") + QString::number(r.height()*(-1)) + QString(" Width: ") + QString::number(r.width()));
    ui->grid1->addWidget(basinInfo,RowCounter,0);

    // expanding in width
    QLabel *lblx = new QLabel(this);
    lblx->setText("expand in width x space with y steps");
    ui->grid1->addWidget(lblx,RowCounter+1,0);
    lblx->show();

    QLineEdit *xadd = new QLineEdit(this);
    xadd->setText("0");
    ui->grid1->addWidget(xadd,RowCounter+1,1);
    xadd->show();

    QLineEdit *xstep = new QLineEdit(this);
    xstep->setText("0");
    ui->grid1->addWidget(xstep,RowCounter+1,2);
    xstep->show();


    // expanding in heigh
    QLabel *lbly = new QLabel(this);
    lbly->setText("expand in height x space with y steps");
    ui->grid1->addWidget(lbly,RowCounter+2,0);
    lbly->show();

    QLineEdit *yadd = new QLineEdit(this);
    yadd->setText("0");
    ui->grid1->addWidget(yadd,RowCounter+2,1);
    yadd->show();

    QLineEdit *ystep = new QLineEdit(this);
    ui->grid1->addWidget(ystep,RowCounter+2,2);
    ystep->setText("0");
    ystep->show();

}



QLineEdit *SceneSampler::LayoutItemToLineEdit(QLayoutItem *qli)
{
    return qobject_cast<QLineEdit *>(qli->widget());
}

void SceneSampler::on_buttonBox_accepted()
{

    QList<basin> basins;
    int RowCounter = 0;
    for (int i = 0; i < BasinCounter; i++) {
        basin b;
        b.xadd = LayoutItemToLineEdit(ui->grid1->itemAtPosition(RowCounter+1,1))->text().toDouble();
        b.xstep = LayoutItemToLineEdit(ui->grid1->itemAtPosition(RowCounter+1,2))->text().toDouble();
        b.yadd = LayoutItemToLineEdit(ui->grid1->itemAtPosition(RowCounter+2,1))->text().toDouble();
        b.ystep = LayoutItemToLineEdit(ui->grid1->itemAtPosition(RowCounter+2,2))->text().toDouble();
        basins.append(b);
        RowCounter += 3;
    }

    std::vector<QRectF> rects = this->s->rects;
    recLoop(rects,rects,this->s->lines,basins,0);    //first time calling recursion at pos 0
}

void SceneSampler::recLoop(std::vector<QRectF> rects,std::vector<QRectF> OriginalRects,std::vector<QLineF> OriginalLines, QList<basin> basins, int ptr)
{
    if(ptr == basins.size()){ // base case of recursion
        // when basin ptr is at last basin write scene and finish remaining recursions
        this->SampleSceneCounter ++;
        this->s->rects = rects;

        qDebug()<<"lines";
        // check what line endpoints have to be moved
        for(int i = 0; i < this->s->lines.size();i++) {
            // check both ends of all lines with the top left/right of original rects
            // and then change new lines to points of new rects
            QLineF l = OriginalLines.at(i);

            int pos = PointAtTopLeftOfRect(l.p1(), OriginalRects);
            if(pos >= 0){
                QRectF r = rects.at(pos);
                l.setP1(r.topLeft());
            }

            pos = PointAtTopRightOfRect(l.p1(), OriginalRects);
            if(pos >= 0){
                QRectF r = rects.at(pos);
                l.setP1(r.topRight());
            }

            pos = PointAtTopLeftOfRect(l.p2(), OriginalRects);
            if(pos >= 0){
                QRectF r = rects.at(pos);
                l.setP2(r.topLeft());
            }

            pos = PointAtTopRightOfRect(l.p2(), OriginalRects);
            if(pos >= 0){
                QRectF r = rects.at(pos);
                l.setP2(r.topRight());
            }

            //delete old and insert new line
            this->s->lines.erase(this->s->lines.begin()+i);
            this->s->lines.insert(this->s->lines.begin()+i,l);
        }
        qDebug()<<"checked";

        //save new scene
        export_scene_to_particle_json(this->s,QString("sampleScene" + QString::number(this->SampleSceneCounter) + ".json"));
        qDebug()<< "scene ";
        BOOST_FOREACH(QRectF &r, rects) {
            qDebug() << r.topLeft().x()<<"/"<<r.topLeft().y();
            qDebug() << r.bottomRight().x()<<"/"<<r.bottomRight().y();
        }
        return;
    }
    // get new basins and rects
    basin b = basins.at(ptr);
    QRectF r = rects.at(ptr);
    QRectF tmp = r;

    // loop over widths and heights and call recursion
    for(double i = 0; i < b.xadd; i+=b.xstep){
        tmp.setRight(tmp.right()+i);
        for (double j = 0; j < b.yadd; j+=b.ystep) {
            tmp.setTop(tmp.top()+j);
            rects.erase(rects.begin()+ptr);
            rects.insert(rects.begin()+ptr,tmp);
            recLoop(rects,OriginalRects,OriginalLines,basins,ptr+1);
        }
        tmp.setTop(r.top());
    }
}

int SceneSampler::PointAtTopLeftOfRect(QPointF p, std::vector<QRectF> OriginalRects)
{
    int counter = 0;
    BOOST_FOREACH(QRectF &r, OriginalRects) {
        if(p.rx() == r.topLeft().rx() && p.ry() == r.topLeft().ry()){
            return counter;
        }
        counter++;
    }
    return -1;
}

int SceneSampler::PointAtTopRightOfRect(QPointF p, std::vector<QRectF> OriginalRects)
{
    int counter = 0;
    BOOST_FOREACH(QRectF &r, OriginalRects) {
        if(p.rx() == r.topRight().rx() && p.ry() == r.topRight().ry()){
            return counter;
        }
        counter++;
    }
    return -1;
}

















