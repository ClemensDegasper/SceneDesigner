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
    int counter = 0;
    BOOST_FOREACH(basin b, basins){
        QRectF r = this->s->rects.at(counter);
        QRectF tmp=r;
        for (double i = 0; i < b.xadd; i+=b.xstep) {
            tmp.setRight(r.right()+i);
            for (double j = 0; j < b.yadd; j+=b.ystep) {
                tmp.setTop(r.top()+j);
                qDebug() << tmp.topLeft().x()<<"/"<<tmp.topLeft().y();
                qDebug() << tmp.bottomRight().x()<<"/"<<tmp.bottomRight().y();
            }
            tmp.setTop(r.top());
        }
    }
}






















