#pragma once

#include <QPointF>
#include <QString>
#include <QWidget>

#include "ArmTypes.h"
#include "TrajectoryPlanner.h"

class QPainter;

// 机械臂仿真视图：绘制工作空间网格、当前机械臂、规划轨迹与目标点。
// 视图只负责显示，不修改机械臂状态；所有坐标单位与算法库一致。
class ArmCanvas : public QWidget
{
    Q_OBJECT
public:
    explicit ArmCanvas(QWidget* parent = nullptr);

    // 当前机械臂状态（由 MainWindow::DrawRobot 写入）。
    void SetArmState(ArmLengthStruct length, ArmAngleStruct angle, ArmPosStruct pos);
    // 规划轨迹（由 MainWindow::DrawTrajectory 写入），空轨迹表示不绘制。
    void SetTrajectory(const TRAJECTORY& trajectory);
    // 当前动画帧，超出轨迹范围时忽略。
    void SetCurrentFrame(int frame);
    // 逆解 / 笛卡尔规划目标点标记。
    void SetTargetMarker(Point2D target, bool visible);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    struct View
    {
        double scale = 1.0;   // 世界单位 -> 屏幕像素
        QPointF origin;       // 世界原点在控件中的屏幕坐标
        double extent = 100.0; // 视图显示的世界半宽（对称）
    };

    View ComputeView() const;
    // 重新计算机械臂 / 轨迹 / 目标点所需的视图范围，只在数据变化时调用。
    void RefreshWorldExtent();
    QPointF ToScreen(const View& view, Point2D point) const;

    void DrawGrid(QPainter& painter, const View& view) const;
    void DrawWorkspace(QPainter& painter, const View& view) const;
    void DrawTrajectory(QPainter& painter, const View& view) const;
    void DrawTarget(QPainter& painter, const View& view) const;
    void DrawRobot(QPainter& painter, const View& view) const;
    void DrawOverlay(QPainter& painter, const View& view) const;
    void DrawAngleArcs(QPainter& painter, const View& view) const;

    // 选择 1/2/5×10^n 形式的网格间距，使屏幕间距接近 60 像素。
    static double PickGridStep(double scale);
    static QString FormatTick(double value, double step);

    ArmLengthStruct armLength{100.0, 100.0};
    ArmAngleStruct armAngle{0.0, 0.0};
    ArmPosStruct armPos{};
    TRAJECTORY trajectory;
    int currentFrame = -1;
    Point2D target{};
    bool targetVisible = false;
    double cachedExtent = 220.0;   // 当前数据所需的世界半宽缓存，避免每帧重算
    double trajectoryExtent = 0.0; // 轨迹点范围缓存，只在 SetTrajectory 时重算
};
