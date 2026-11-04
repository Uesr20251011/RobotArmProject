#pragma once

#include <QMainWindow>
#include <QTimer>

#include "Kinematics.h"
#include "RobotArm.h"
#include "TrajectoryPlanner.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    // 参数设置
    void OnSetParameters();

    // 运动学计算
    void OnForwardSolve();
    void OnInverseSolve();

    // 轨迹规划
    void OnPlanJoint();
    void OnPlanCartesian();

    // 动画控制
    void OnPlay();
    void OnPause();
    void OnNextFrame();
    void OnResetAnimation();
    void OnSeekFrame(int frame);

private:
    // 刷新界面中的机械臂信息
    void UpdateDisplay();

    // 绘制当前机械臂
    void DrawRobot();

    // 绘制规划好的轨迹
    void DrawTrajectory();

    // 把机械臂的长度与角度同步回左侧输入控件
    void SyncControlsFromArm();
    // 只同步关节角度（动画帧更新用，避免覆盖尚未应用的长度输入）
    void SyncAngleControlsFromArm();
    // 播放动画期间禁用参数输入，避免未应用的手工输入被帧更新覆盖
    void SetParameterControlsEnabled(bool enabled);
    // 停止动画定时器
    void StopAnimation();
    // 把第 frame 个轨迹点的关节角应用到机械臂
    void ApplyFrame(int frame);
    // 同步帧计数、进度条与动画按钮状态
    void UpdateFrameControls();
    // 清空轨迹并复位动画
    void ClearTrajectory();
    // 播放定时器间隔（按采样间隔和播放速度计算）
    int PlaybackIntervalMs() const;
    // 读取并校验运动时间与采样间隔
    bool ReadTrajectoryTiming(double& duration, double& timeStep);
    // 规划前确认输入框中的连杆长度已经应用，避免 IK / 规划使用不同几何
    bool EnsureArmLengthsApplied();
    // 检查规划器输出是否包含有效数值，避免 NaN / Inf 进入绘制与机械臂状态
    bool ValidateTrajectory(const TRAJECTORY& plannedTrajectory) const;
    // 当前选择的肘部构型，true 表示肘上（对应 elbowUp）
    bool ReadElbowUp() const;
    static QString DescribeIkStatus(IKStatus status);
    // 小屏幕上把窗口限制在可用桌面范围内
    void FitWindowToScreen();

    Ui::MainWindow* ui;

    RobotArm arm;              // 当前机械臂
    TrajectoryPlanner planner; // 轨迹规划器
    TRAJECTORY trajectory;     // 当前运动轨迹

    QTimer* timer;             // 动画定时器
    int currentFrame;          // 当前播放帧
    double playbackTimeStep;   // 生成轨迹时使用的采样间隔（秒）
    bool trajectoryDirty;      // 轨迹内容变化后需要同步给画布
    Point2D targetMarker;      // 上次逆解 / 笛卡尔规划的目标点
    bool targetMarkerVisible;  // 是否显示目标点标记
};
