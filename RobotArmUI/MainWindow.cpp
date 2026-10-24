#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QStatusBar>

namespace {
QString DescribeStatus(Status status)
{
    switch (status) {
    case Status::Success: return QObject::tr("执行成功");
    case Status::InvalidParameter: return QObject::tr("机械臂或时间参数不合法");
    case Status::InvalidInput: return QObject::tr("输入包含无效数值");
    case Status::NotInitialized: return QObject::tr("尚未初始化");
    case Status::Unreachable: return QObject::tr("目标位置不可达");
    case Status::NonUniqueSolution: return QObject::tr("无法确定唯一解");
    case Status::NumericalError: return QObject::tr("计算结果超出有效数值范围");
    case Status::NotImplemented: return QObject::tr("该算法尚未实现");
    }
    return QObject::tr("未知状态");
}
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), timer(new QTimer(this)), currentFrame(0)
{
    ui->setupUi(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::OnNextFrame);
    connect(ui->forwardButton, &QPushButton::clicked, this, &MainWindow::OnForwardSolve);
    UpdateDisplay();
    statusBar()->showMessage(tr("工程骨架已加载：Common / Kinematics / Trajectory"));
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::UpdateDisplay()
{
    const auto length = arm.QueryLength();
    const auto angle = arm.QueryAngle();
    const auto pos = arm.QueryPos();
    ui->stateLabel->setText(tr("连杆长度：%1 / %2\n关节角度（弧度）：%3 / %4\n肘关节：(%5, %6)\n末端：(%7, %8)")
        .arg(length.length1).arg(length.length2).arg(angle.q1).arg(angle.q2)
        .arg(pos.L1.x).arg(pos.L1.y).arg(pos.L2.x).arg(pos.L2.y));
    DrawRobot();
    DrawTrajectory();
}

void MainWindow::OnForwardSolve()
{
    const auto result = ForwardSolve(arm.QueryLength(), arm.QueryAngle());
    if (result.status != Status::Success) {
        statusBar()->showMessage(tr("正运动学失败：%1。机械臂状态保持不变。")
            .arg(DescribeStatus(result.status)));
        return;
    }
    const auto status = arm.SetPara(arm.QueryLength(), arm.QueryAngle(), result.pos);
    if (status != Status::Success) {
        statusBar()->showMessage(tr("机械臂状态更新失败：%1。").arg(DescribeStatus(status)));
        return;
    }
    UpdateDisplay();
    statusBar()->showMessage(tr("已通过运动学 DLL 完成正运动学调用。"));
}

void MainWindow::OnSetParameters() { statusBar()->showMessage(tr("参数输入由界面负责人实现。")); }
void MainWindow::OnInverseSolve() { statusBar()->showMessage(tr("逆运动学由运动学负责人实现。")); }
void MainWindow::OnPlanJoint() { statusBar()->showMessage(tr("关节空间规划由轨迹规划负责人实现。")); }
void MainWindow::OnPlanCartesian() { statusBar()->showMessage(tr("笛卡尔空间规划由轨迹规划负责人实现。")); }
void MainWindow::OnPlay() { statusBar()->showMessage(tr("轨迹动画由界面负责人实现。")); }
void MainWindow::OnPause() { timer->stop(); }
void MainWindow::OnNextFrame() { timer->stop(); }
void MainWindow::DrawRobot() { /* UI workstream: render arm.QueryPos(). */ }
void MainWindow::DrawTrajectory() { /* UI workstream: render trajectory. */ }
