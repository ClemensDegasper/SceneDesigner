#include "designer.h"
#include "ui_designer.h"
#include <QShortcut>
#include "scenesaver.h"
#include <QSlider>
#include <QDebug>
#include <QTreeWidget>
#include <QFileDialog>
#include <QProcess>
#include "scenesampler.h"
#include <QMouseEvent>
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
    ui->doubleSpinBoxCutoffRadius->setValue(scene->getCutOffRadius());

    this->addAction(ui->action_save);
    this->addAction(ui->actionNewScene);
    this->addAction(ui->actionDec_size);
    this->addAction(ui->actionInc_size);
    this->addAction(ui->actionRun);
    this->addAction(ui->actionAddBoundary);
    this->addAction(ui->actionAddPhase1);
    this->addAction(ui->actionAddPhase2);

    connect(this->ui->SceneSlider, SIGNAL(valueChanged(int)), this, SLOT(SliderValue(int)));

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

void Designer::on_doubleSpinBoxSamplingDistance_valueChanged(double r)
{
    //scene->setCutoffRadius(r);
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
    this->scene->clear();
    if (!open_file.isEmpty()) {
        open_scene(scene, open_file);
    }
}




void Designer::on_buttonClear_released()
{
    this->scene->clear();
}

void Designer::on_buttonExport_released()
{
    QString save_file = QFileDialog::getSaveFileName(this, tr("Save File"),
                                                     "", tr("SPH JSON Files (*.json)"));
    export_scene_to_particle_json(this->scene,save_file);
}


void Designer::on_buttonSample_released()
{
    SceneSampler *window = new SceneSampler(this, this->scene,this->ui->SceneSlider);
    window->show();
}



void Designer::on_doubleSpinBoxCutoffRadius_valueChanged(const QString &arg1)
{
    scene->setCutoffRadius(arg1.toDouble());
}

void Designer::on_buttonRepairCircle_released()
{
    ui->designer_view->setMode(RepairCircle);
}

void Designer::on_buttonRepairSquare_released()
{
    ui->designer_view->setMode(RepairSquare);
}

void Designer::SliderValue(int pos)
{
    this->ui->lblSceneCounter->setText("Scenes: " + QString::number(this->ui->SceneSlider->maximum()));
    QString open_file = "sampleScene" + QString::number(pos) + ".json";

    this->scene->clear();
    if (!open_file.isEmpty()) {
        open_scene(this->scene, open_file);
        qDebug()<< "opening " + open_file;
    }else{
        qDebug()<< open_file + " is empty";
    }

//    QMouseEvent* evt = new QMouseEvent(QEvent::MouseButtonPress,
//    this->rect().center(),Qt::LeftButton, Qt::LeftButton,Qt::NoModifier);
//    this->mouseMoveEvent(evt);
    this->ui->designer_view->update();
}

void Designer::on_buttonPolygon_released()
{
    this->scene->Sim = new LenJonSim();
    ui->designer_view->setMode(RepairPoly);
}

void Designer::on_buttonConvert_released()
{
    QString name = QString("tmp.json");
    export_scene_to_particle_json(this->scene,name);
    this->scene->clear();
    open_scene(this->scene,name);
    this->ui->designer_view->update();
}

void Designer::on_buttonClearSim_released()
{
    if(this->scene->Sim != 0)
        this->scene->Sim->clear();
    this->ui->designer_view->update();
}

void Designer::on_buttonFinish_released()
{
    this->scene->LJSimulationFinished();
}



void Designer::on_buttonInflow_released()
{
    this->ui->designer_view->setMode(PlaceInFlow);
}

void Designer::on_buttonOutflow_released()
{
    this->ui->designer_view->setMode(PlaceOutFlow);
}
