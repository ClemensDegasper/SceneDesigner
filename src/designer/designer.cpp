#include "designer.h"
#include "ui_designer.h"
#include <QShortcut>
#include "scenesaver.h"

#include <QDebug>
#include <QTreeWidget>
#include <QFileDialog>
#include <QProcess>

#include <boost/foreach.hpp>

Designer::Designer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Designer), scene(new Scene()) {
    ui->setupUi(this);
    connect(scene, SIGNAL(changed()), SLOT(sceneChanged()));
    ui->designer_view->setScene(scene);

    ui->doubleSpinBoxWidth->setValue(scene->getWidth());
    ui->doubleSpinBoxHeight->setValue(scene->getHeight());
    ui->doubleSpinBoxSamplingDistance->setValue(scene->getSamplingDistance());

    this->addAction(ui->action_save);
    this->addAction(ui->actionNewScene);
    this->addAction(ui->actionDec_size);
    this->addAction(ui->actionInc_size);
    this->addAction(ui->actionRun);
    this->addAction(ui->actionAddBoundary);
    this->addAction(ui->actionAddPhase1);
    this->addAction(ui->actionAddPhase2);
}

Designer::~Designer() {
    delete ui;
}

void Designer::on_buttonBoundary_clicked() {
    ui->designer_view->setMode(Boundary);
}

void Designer::on_buttonFluid1_clicked() {
    ui->designer_view->setMode(Fluid1);
}

void Designer::on_buttonPan_clicked() {
    ui->designer_view->setMode(Pan);
}

void Designer::on_buttonFluid2_clicked() {
    ui->designer_view->setMode(Fluid2);
}

void Designer::on_buttonLine_clicked()
{
    ui->designer_view->setMode(Line);
}

void Designer::on_buttonErase_clicked() {
    ui->designer_view->setMode(None);
}

void Designer::on_buttonRectangle_released()
{
    ui->designer_view->setMode(Rectangle);
}

void Designer::on_doubleSpinBoxSamplingDistance_editingFinished() {
    scene->setGrid(ui->doubleSpinBoxWidth->value(), ui->doubleSpinBoxHeight->value(), ui->doubleSpinBoxSamplingDistance->value());
}

void Designer::sceneChanged() {
}

void Designer::on_doubleSpinBoxWidth_editingFinished() {
    scene->setGrid(ui->doubleSpinBoxWidth->value(), ui->doubleSpinBoxHeight->value(), ui->doubleSpinBoxSamplingDistance->value());
}

void Designer::on_doubleSpinBoxHeight_editingFinished() {
    scene->setGrid(ui->doubleSpinBoxWidth->value(), ui->doubleSpinBoxHeight->value(), ui->doubleSpinBoxSamplingDistance->value());
}

void Designer::on_doubleSpinBoxAccelerationX_editingFinished() {
    scene->setAccelerationX(ui->doubleSpinBoxAccelerationX->value());
}

void Designer::on_doubleSpinBoxAccelerationY_editingFinished() {
    scene->setAccelerationY(ui->doubleSpinBoxAccelerationY->value());
}

void Designer::on_spinBoxNeighbours_editingFinished() {
    scene->setNeighbours(ui->spinBoxNeighbours->value());
}

void Designer::on_spinBoxC_editingFinished() {
    scene->setC(ui->spinBoxC->value());
}

void Designer::on_doubleSpinBoxAlpha_editingFinished() {
    scene->setAlpha(ui->doubleSpinBoxAlpha->value());
}

void Designer::on_doubleSpinBoxDampingFactor_editingFinished() {
    scene->setDampingFactor(ui->doubleSpinBoxDampingFactor->value());
}

void Designer::on_doubleSpinBoxShepard_editingFinished() {
    scene->setShepard(ui->doubleSpinBoxShepard->value());
}

void Designer::on_doubleSpinBoxXSPH_editingFinished() {
    scene->setXSPH(ui->doubleSpinBoxXSPH->value());
}

void Designer::on_doubleSpinBoxNoSlip_editingFinished() {
    scene->setNoSlip(ui->doubleSpinBoxNoSlip->value());
}

void Designer::on_action_save_triggered() {
    save();
}


void Designer::on_action_open_triggered() {
    open();
}

void Designer::on_actionRun_triggered() {
    save_scene(scene, "/tmp/test.json");
    QStringList args;
    args << "/tmp/test.json";

    QProcess p;
    p.start("../sph", args);
    p.waitForFinished();
}

void Designer::on_actionAddBoundary_triggered() {
    ui->designer_view->setMode(Boundary);
}

void Designer::on_actionAddPhase1_triggered() {
    ui->designer_view->setMode(Fluid1);
}

void Designer::on_actionAddPhase2_triggered() {
    ui->designer_view->setMode(Fluid1);
}

void Designer::on_actionNewScene_triggered() {
    scene->clear();
    ui->designer_view->sceneUpdated();
}

void Designer::on_actionInc_size_triggered() {
    ui->designer_view->setSize(ui->designer_view->getSize() + 1);
}

void Designer::on_actionDec_size_triggered() {
    ui->designer_view->setSize(ui->designer_view->getSize() - 1);
}

void Designer::on_actionCheck_particles_triggered() {

}

void Designer::on_saveButton_clicked() {
    save();
}

void Designer::on_openButton_clicked() {
    open();
}

inline
void Designer::save() {
    QString save_file = QFileDialog::getSaveFileName(this, tr("Save File"),
                                                     "", tr("SPH JSON Files (*.json)"));

    if (!save_file.isEmpty()) {
        save_scene(scene, save_file);
    }
}

inline
void Designer::open() {
    QString open_file = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                     "", tr("SPH JSON Files (*.json)"));

    if (!open_file.isEmpty()) {
        open_scene(scene, open_file);
    }
}



