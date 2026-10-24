#pragma once
#include <QMainWindow>
#include <QTimer>
#include "RobotArm.h"
#include "TrajectoryPlanner.h"
#include "Kinematics.h"

namespace Ui { class MainWindow; }

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;
private slots:
    void OnSetParameters();
    void OnForwardSolve();
    void OnInverseSolve();
    void OnPlanJoint();
    void OnPlanCartesian();
    void OnPlay();
    void OnPause();
    void OnNextFrame();
private:
    void UpdateDisplay();
    void DrawRobot();
    void DrawTrajectory();
    Ui::MainWindow* ui;
    RobotArm arm;
    TrajectoryPlanner planner;
    TRAJECTORY trajectory;
    QTimer* timer;
    int currentFrame;
};
