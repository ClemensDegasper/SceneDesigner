#ifndef DESIGNER_H
#define DESIGNER_H

#include <QWidget>

#include <scene.h>

namespace Ui {
class Designer;
}

class QTreeWidgetItem;

class Designer : public QWidget {
    Q_OBJECT

public:
    explicit Designer(QWidget *parent = 0);
    ~Designer();

    Scene *getScene() {
        return scene;
    }

    inline void save();
    inline void open();

private slots:
    void on_buttonBoundary_clicked();
    void on_buttonFluid1_clicked();
    void on_buttonFluid2_clicked();
    void on_buttonPan_clicked();
    void on_buttonErase_clicked();

    void sceneChanged();

    void on_doubleSpinBoxSamplingDistance_editingFinished();
    void on_doubleSpinBoxWidth_editingFinished();
    void on_doubleSpinBoxHeight_editingFinished();
    void on_doubleSpinBoxAccelerationX_editingFinished();
    void on_doubleSpinBoxAccelerationY_editingFinished();
    void on_spinBoxNeighbours_editingFinished();
    void on_spinBoxC_editingFinished();
    void on_doubleSpinBoxAlpha_editingFinished();
    void on_doubleSpinBoxDampingFactor_editingFinished();
    void on_doubleSpinBoxShepard_editingFinished();
    void on_doubleSpinBoxXSPH_editingFinished();
    void on_doubleSpinBoxNoSlip_editingFinished();

    void on_action_save_triggered();
    void on_action_open_triggered();
    void on_actionNewScene_triggered();
    void on_actionInc_size_triggered();
    void on_actionRun_triggered();
    void on_actionDec_size_triggered();
    void on_actionCheck_particles_triggered();
    void on_actionAddBoundary_triggered();
    void on_actionAddPhase1_triggered();
    void on_actionAddPhase2_triggered();

    void on_saveButton_clicked();
    void on_openButton_clicked();

    void on_buttonRectangle_released();

    void on_buttonLine_clicked();

    void on_buttonClear_released();

    void on_buttonExport_released();

    void on_doubleSpinBoxSamplingDistance_valueChanged(double r);

    void on_buttonSample_released();

    void on_doubleSpinBoxCutoffRadius_valueChanged(const QString &arg1);

    void on_buttonRepairCircle_released();

    void on_buttonRepairSquare_released();

    void SliderValue(int pos);



    void on_buttonPolygon_released();

    void on_buttonConvert_released();

    void on_buttonClearSim_released();

    void on_buttonFinish_released();


    void on_buttonInflow_released();

    void on_buttonOutflow_released();

private:
    Ui::Designer *ui;
    Scene *scene;
};

#endif // DESIGNER_H
