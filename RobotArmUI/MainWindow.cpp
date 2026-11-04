#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QScreen>
#include <QSignalBlocker>
#include <QSlider>
#include <QStatusBar>
#include <QtMath>
#include <algorithm>
#include <cmath>

namespace {
constexpr double kDefaultTimeStep = 0.02; // 秒，与界面默认采样间隔一致
constexpr int kMaxSampleCount = 20000;    // 防止一次生成过多采样点卡住界面
constexpr int kMinTimerIntervalMs = 5;    // 允许 4× 播放（默认采样间隔下为 5 ms）
constexpr int kMaxTimerIntervalMs = 2000; // 允许 0.5× 播放 1 s 采样间隔

QString FormatNumber(double value, int decimals)
{
    if (qAbs(value) < 5e-10) {
        value = 0.0;
    }
    return QString::number(value, 'f', decimals);
}
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , timer(new QTimer(this))
    , currentFrame(0)
    , playbackTimeStep(kDefaultTimeStep)
    , trajectoryDirty(true)
    , targetMarker{0.0, 0.0}
    , targetMarkerVisible(false)
{
    ui->setupUi(this);
    FitWindowToScreen();
    ui->splitter->setSizes({470, 860});
    ui->frameSlider->setPageStep(10);

    timer->setTimerType(Qt::PreciseTimer);
    connect(timer, &QTimer::timeout, this, &MainWindow::OnNextFrame);

    // ④ 动画播放速度倍率。
    ui->speedCombo->addItem(tr("0.5×"), 0.5);
    ui->speedCombo->addItem(tr("1×"), 1.0);
    ui->speedCombo->addItem(tr("2×"), 2.0);
    ui->speedCombo->addItem(tr("4×"), 4.0);
    ui->speedCombo->setCurrentIndex(1);

    // ② 逆运动学的肘上 / 肘下单选按钮在 Designer 界面中已经设置并互斥。

    connect(ui->applyButton, &QPushButton::clicked, this, &MainWindow::OnSetParameters);
    connect(ui->forwardButton, &QPushButton::clicked, this, &MainWindow::OnForwardSolve);
    connect(ui->inverseButton, &QPushButton::clicked, this, &MainWindow::OnInverseSolve);
    connect(ui->jointPlanButton, &QPushButton::clicked, this, &MainWindow::OnPlanJoint);
    connect(ui->cartesianPlanButton, &QPushButton::clicked, this, &MainWindow::OnPlanCartesian);
    connect(ui->playButton, &QPushButton::clicked, this, &MainWindow::OnPlay);
    connect(ui->pauseButton, &QPushButton::clicked, this, &MainWindow::OnPause);
    connect(ui->nextButton, &QPushButton::clicked, this, &MainWindow::OnNextFrame);
    connect(ui->resetButton, &QPushButton::clicked, this, &MainWindow::OnResetAnimation);
    connect(ui->frameSlider, &QSlider::valueChanged, this, &MainWindow::OnSeekFrame);
    connect(ui->speedCombo, &QComboBox::currentIndexChanged, this, [this](int) {
        if (timer->isActive()) {
            timer->start(PlaybackIntervalMs());
        }
    });

    SyncControlsFromArm();
    UpdateDisplay();
    statusBar()->showMessage(tr("就绪：设置参数后点击按钮，右侧视图会实时显示机械臂与轨迹。"));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::FitWindowToScreen()
{
    const QScreen* currentScreen = screen();
    if (!currentScreen) {
        return;
    }
    const QRect available = currentScreen->availableGeometry();
    if (!available.isValid()) {
        return;
    }
    const int width = qMax(minimumWidth(), qMin(1360, available.width() - 40));
    const int height = qMax(minimumHeight(), qMin(940, available.height() - 60));
    resize(width, height);
}

void MainWindow::SyncControlsFromArm()
{
    const ArmLengthStruct length = arm.QueryLength();
    const ArmAngleStruct angle = arm.QueryAngle();
    ui->length1Spin->setValue(length.length1);
    ui->length2Spin->setValue(length.length2);
    ui->q1Spin->setValue(qRadiansToDegrees(angle.q1));
    ui->q2Spin->setValue(qRadiansToDegrees(angle.q2));
}

void MainWindow::SyncAngleControlsFromArm()
{
    const ArmAngleStruct angle = arm.QueryAngle();
    ui->q1Spin->setValue(qRadiansToDegrees(angle.q1));
    ui->q2Spin->setValue(qRadiansToDegrees(angle.q2));
}

void MainWindow::SetParameterControlsEnabled(bool enabled)
{
    // 播放期间锁住参数输入，避免逐帧刷新覆盖用户尚未应用的修改。
    ui->paramGroup->setEnabled(enabled);
}

void MainWindow::StopAnimation()
{
    timer->stop();
}

void MainWindow::ClearTrajectory()
{
    StopAnimation();
    trajectory.clear();
    currentFrame = 0;
    playbackTimeStep = kDefaultTimeStep;
    trajectoryDirty = true;
}

void MainWindow::ApplyFrame(int frame)
{
    if (trajectory.empty()) {
        currentFrame = 0;
        return;
    }
    currentFrame = qBound(0, frame, static_cast<int>(trajectory.size()) - 1);
    // 用轨迹中的关节角更新机械臂，Common DLL 会重新调用正运动学更新位置。
    arm.SetPara(arm.QueryLength(), trajectory[static_cast<size_t>(currentFrame)].angle);
    SyncAngleControlsFromArm();
}

void MainWindow::UpdateFrameControls()
{
    const int frameCount = static_cast<int>(trajectory.size());
    const bool hasTrajectory = frameCount > 0;
    const bool playing = timer->isActive();
    {
        const QSignalBlocker blocker(ui->frameSlider);
        ui->frameSlider->setRange(0, hasTrajectory ? frameCount - 1 : 0);
        ui->frameSlider->setValue(hasTrajectory ? currentFrame : 0);
    }
    ui->frameSlider->setEnabled(hasTrajectory);
    ui->frameLabel->setText(hasTrajectory
            ? tr("第 %1 / %2 帧").arg(currentFrame + 1).arg(frameCount)
            : tr("第 0 / 0 帧"));
    ui->playButton->setEnabled(hasTrajectory && !playing);
    ui->pauseButton->setEnabled(hasTrajectory && playing);
    ui->nextButton->setEnabled(hasTrajectory);
    ui->resetButton->setEnabled(hasTrajectory);
    SetParameterControlsEnabled(!playing);
    if (hasTrajectory) {
        ui->trajectoryLabel->setText(
            tr("轨迹共 %1 个点，时长 %2 s，采样间隔 %3 s；点击“播放”查看动画。")
                .arg(frameCount)
                .arg(FormatNumber(trajectory.back().time, 3))
                .arg(FormatNumber(playbackTimeStep, 3)));
    } else {
        ui->trajectoryLabel->setText(tr("当前没有轨迹：请先成功生成一条轨迹。"));
    }
}

int MainWindow::PlaybackIntervalMs() const
{
    double speed = ui->speedCombo->currentData().toDouble();
    if (!std::isfinite(speed) || speed <= 0.0) {
        speed = 1.0;
    }
    const double interval = playbackTimeStep * 1000.0 / speed;
    if (!std::isfinite(interval) || interval <= 0.0) {
        return 20;
    }
    return qBound(kMinTimerIntervalMs, static_cast<int>(std::lround(interval)), kMaxTimerIntervalMs);
}

bool MainWindow::ReadTrajectoryTiming(double& duration, double& timeStep)
{
    duration = ui->durationSpin->value();
    timeStep = ui->timeStepSpin->value();
    if (!std::isfinite(duration) || !std::isfinite(timeStep) || duration <= 0.0 || timeStep <= 0.0) {
        statusBar()->showMessage(tr("规划失败：运动时间和采样间隔必须大于 0。"));
        return false;
    }
    if (duration / timeStep > kMaxSampleCount) {
        statusBar()->showMessage(tr("规划失败：采样点过多，请增大采样间隔或减小运动时间。"));
        return false;
    }
    return true;
}

bool MainWindow::EnsureArmLengthsApplied()
{
    const ArmLengthStruct length = arm.QueryLength();
    const double scale = (std::max)({(std::max)(length.length1, length.length2), 1.0});
    const double tolerance = scale * 1e-9;
    const bool changed = qAbs(length.length1 - ui->length1Spin->value()) > tolerance
        || qAbs(length.length2 - ui->length2Spin->value()) > tolerance;
    if (changed) {
        statusBar()->showMessage(tr("规划失败：连杆长度尚未应用，请先点击“应用参数”。"));
        return false;
    }
    return true;
}

bool MainWindow::ValidateTrajectory(const TRAJECTORY& plannedTrajectory) const
{
    if (plannedTrajectory.empty()) {
        return false;
    }
    for (const auto& point : plannedTrajectory) {
        if (!std::isfinite(point.time) || !std::isfinite(point.angle.q1) || !std::isfinite(point.angle.q2)
            || !std::isfinite(point.pos.L1.x) || !std::isfinite(point.pos.L1.y)
            || !std::isfinite(point.pos.L2.x) || !std::isfinite(point.pos.L2.y)) {
            return false;
        }
    }
    return true;
}

bool MainWindow::ReadElbowUp() const
{
    return ui->elbowUpRadio->isChecked();
}

QString MainWindow::DescribeIkStatus(IKStatus status)
{
    switch (status) {
    case IKStatus::Success:
        return tr("成功");
    case IKStatus::InvalidLength:
        return tr("连杆长度无效");
    case IKStatus::Unreachable:
        return tr("目标点超出工作空间");
    case IKStatus::SingularStraight:
        return tr("奇异位形：两连杆完全伸直");
    case IKStatus::SingularFolded:
        return tr("奇异位形：两连杆完全折叠");
    }
    return tr("未知状态");
}

void MainWindow::UpdateDisplay()
{
    const ArmLengthStruct length = arm.QueryLength();
    const ArmAngleStruct angle = arm.QueryAngle();
    const ArmPosStruct pos = arm.QueryPos();
    ui->stateLabel->setText(
        tr("连杆长度：L1 = %1，L2 = %2\n"
           "关节角度：q1 = %3 rad（%4°），q2 = %5 rad（%6°）\n"
           "肘关节 L1：( %7, %8 )\n"
           "末端执行器 L2：( %9, %10 )")
            .arg(FormatNumber(length.length1, 3))
            .arg(FormatNumber(length.length2, 3))
            .arg(FormatNumber(angle.q1, 4))
            .arg(FormatNumber(qRadiansToDegrees(angle.q1), 2))
            .arg(FormatNumber(angle.q2, 4))
            .arg(FormatNumber(qRadiansToDegrees(angle.q2), 2))
            .arg(FormatNumber(pos.L1.x, 3))
            .arg(FormatNumber(pos.L1.y, 3))
            .arg(FormatNumber(pos.L2.x, 3))
            .arg(FormatNumber(pos.L2.y, 3)));
    DrawRobot();
    DrawTrajectory();
    UpdateFrameControls();
}

void MainWindow::DrawRobot()
{
    ui->canvas->SetArmState(arm.QueryLength(), arm.QueryAngle(), arm.QueryPos());
}

void MainWindow::DrawTrajectory()
{
    if (trajectoryDirty) {
        ui->canvas->SetTrajectory(trajectory);
        trajectoryDirty = false;
    }
    ui->canvas->SetCurrentFrame(trajectory.empty() ? -1 : currentFrame);
    ui->canvas->SetTargetMarker(targetMarker, targetMarkerVisible);
}

void MainWindow::OnSetParameters()
{
    const ArmLengthStruct length{ui->length1Spin->value(), ui->length2Spin->value()};
    if (length.length1 <= 0.0 || length.length2 <= 0.0) {
        statusBar()->showMessage(tr("参数无效：连杆长度必须大于 0。"));
        return;
    }
    const ArmAngleStruct angle{qDegreesToRadians(ui->q1Spin->value()),
                               qDegreesToRadians(ui->q2Spin->value())};
    ClearTrajectory();
    targetMarkerVisible = false;
    arm.SetPara(length, angle);
    SyncControlsFromArm();
    UpdateDisplay();
    statusBar()->showMessage(tr("已应用参数：L1 = %1，L2 = %2，q1 = %3°，q2 = %4°。")
                                 .arg(FormatNumber(length.length1, 3))
                                 .arg(FormatNumber(length.length2, 3))
                                 .arg(FormatNumber(ui->q1Spin->value(), 2))
                                 .arg(FormatNumber(ui->q2Spin->value(), 2)));
}

void MainWindow::OnForwardSolve()
{
    const ArmLengthStruct length{ui->length1Spin->value(), ui->length2Spin->value()};
    if (length.length1 <= 0.0 || length.length2 <= 0.0) {
        statusBar()->showMessage(tr("正运动学失败：连杆长度必须大于 0。"));
        return;
    }
    const ArmAngleStruct angle{qDegreesToRadians(ui->q1Spin->value()),
                               qDegreesToRadians(ui->q2Spin->value())};
    // 直接调用 Kinematics DLL 的导出函数，并用显式位置重设公共状态。
    const ArmPosStruct pos = ForwardSolve(length, angle);
    ClearTrajectory();
    targetMarkerVisible = false;
    arm.SetPara(length, angle, pos);
    SyncControlsFromArm();
    UpdateDisplay();
    statusBar()->showMessage(tr("正运动学：肘关节 (%1, %2)，末端 (%3, %4)。")
                                 .arg(FormatNumber(pos.L1.x, 3))
                                 .arg(FormatNumber(pos.L1.y, 3))
                                 .arg(FormatNumber(pos.L2.x, 3))
                                 .arg(FormatNumber(pos.L2.y, 3)));
}

void MainWindow::OnInverseSolve()
{
    // 先停住正在播放的动画：失败提示之后机械臂不应继续运动。
    StopAnimation();
    const ArmLengthStruct length{ui->length1Spin->value(), ui->length2Spin->value()};
    const Point2D target{ui->targetXSpin->value(), ui->targetYSpin->value()};
    const bool elbowUp = ReadElbowUp();
    // 失败时算法约定保持输出角度不变，这里用当前角度作为初始值。
    ArmAngleStruct result = arm.QueryAngle();
    const IKStatus status = InverseSolve(length, target, elbowUp, result);
    targetMarker = target;
    targetMarkerVisible = true;
    if (status != IKStatus::Success) {
        // 机械臂状态保持不变，只暂停动画并更新目标点标记。
        UpdateDisplay();
        statusBar()->showMessage(tr("逆运动学求解失败：%1。").arg(DescribeIkStatus(status)));
        return;
    }
    ClearTrajectory();
    arm.SetPara(length, result);
    SyncControlsFromArm();
    UpdateDisplay();
    statusBar()->showMessage(tr("逆运动学（%1）成功：q1 = %2°，q2 = %3°。")
                                 .arg(elbowUp ? tr("肘上") : tr("肘下"))
                                 .arg(FormatNumber(qRadiansToDegrees(result.q1), 2))
                                 .arg(FormatNumber(qRadiansToDegrees(result.q2), 2)));
}

void MainWindow::OnPlanJoint()
{
    StopAnimation();
    double duration = 0.0;
    double timeStep = 0.0;
    if (!ReadTrajectoryTiming(duration, timeStep)) {
        UpdateFrameControls();
        return;
    }
    if (!EnsureArmLengthsApplied()) {
        UpdateFrameControls();
        return;
    }
    const ArmAngleStruct target{qDegreesToRadians(ui->targetQ1Spin->value()),
                                qDegreesToRadians(ui->targetQ2Spin->value())};
    targetMarkerVisible = false;
    trajectoryDirty = true; // 规划器会直接写入 trajectory
    const bool planned = planner.PlanJoint(arm, target, duration, timeStep, trajectory);
    if (!planned || !ValidateTrajectory(trajectory)) {
        // 失败时无论规划器是否清理，界面都不保留旧轨迹。
        trajectory.clear();
        currentFrame = 0;
        UpdateDisplay();
        statusBar()->showMessage(
            tr("关节空间轨迹规划失败：未生成有效轨迹，请检查目标角度与时间参数，或确认轨迹规划模块已实现。"));
        return;
    }
    currentFrame = 0;
    playbackTimeStep = timeStep;
    ApplyFrame(0);
    UpdateDisplay();
    statusBar()->showMessage(tr("关节空间规划成功：共 %1 个轨迹点，时长 %2 s。点击“播放”查看动画。")
                                 .arg(trajectory.size())
                                 .arg(FormatNumber(trajectory.back().time, 3)));
}

void MainWindow::OnPlanCartesian()
{
    StopAnimation();
    double duration = 0.0;
    double timeStep = 0.0;
    if (!ReadTrajectoryTiming(duration, timeStep)) {
        UpdateFrameControls();
        return;
    }
    if (!EnsureArmLengthsApplied()) {
        UpdateFrameControls();
        return;
    }
    const Point2D target{ui->targetXSpin->value(), ui->targetYSpin->value()};
    const bool elbowUp = ReadElbowUp();
    targetMarker = target;
    targetMarkerVisible = true;
    trajectoryDirty = true; // 规划器会直接写入 trajectory
    const bool planned = planner.PlanCartesian(arm, target, elbowUp, duration, timeStep, trajectory);
    if (!planned || !ValidateTrajectory(trajectory)) {
        // 失败时无论规划器是否清理，界面都不保留旧轨迹。
        trajectory.clear();
        currentFrame = 0;
        UpdateDisplay();
        statusBar()->showMessage(
            tr("笛卡尔直线规划失败：目标点可能不可达（逆解失败），或运动时间 / 采样间隔无效。"));
        return;
    }
    currentFrame = 0;
    playbackTimeStep = timeStep;
    ApplyFrame(0);
    UpdateDisplay();
    statusBar()->showMessage(tr("笛卡尔直线规划成功：共 %1 个轨迹点，时长 %2 s。点击“播放”查看动画。")
                                 .arg(trajectory.size())
                                 .arg(FormatNumber(trajectory.back().time, 3)));
}

void MainWindow::OnPlay()
{
    if (trajectory.empty()) {
        statusBar()->showMessage(tr("没有可播放的轨迹：请先生成轨迹。"));
        return;
    }
    if (timer->isActive()) {
        return;
    }
    bool restarted = false;
    if (currentFrame >= static_cast<int>(trajectory.size()) - 1) {
        currentFrame = 0;
        ApplyFrame(0);
        restarted = true;
    }
    const int interval = PlaybackIntervalMs();
    timer->start(interval);
    UpdateDisplay();
    statusBar()->showMessage(restarted
            ? tr("轨迹已播放完毕，重新从头播放：共 %1 帧，帧间隔 %2 ms。")
                  .arg(trajectory.size())
                  .arg(interval)
            : tr("正在播放轨迹动画：共 %1 帧，帧间隔 %2 ms。")
                  .arg(trajectory.size())
                  .arg(interval));
}

void MainWindow::OnPause()
{
    if (!timer->isActive()) {
        statusBar()->showMessage(tr("动画当前未在播放。"));
        return;
    }
    StopAnimation();
    UpdateFrameControls();
    statusBar()->showMessage(tr("已暂停：第 %1 / %2 帧。")
                                 .arg(currentFrame + 1)
                                 .arg(trajectory.size()));
}

void MainWindow::OnNextFrame()
{
    if (trajectory.empty()) {
        timer->stop();
        UpdateFrameControls();
        statusBar()->showMessage(tr("没有可播放的轨迹：请先生成轨迹。"));
        return;
    }
    const bool playing = timer->isActive();
    const int lastFrame = static_cast<int>(trajectory.size()) - 1;
    if (currentFrame >= lastFrame) {
        timer->stop();
        UpdateFrameControls();
        if (playing) {
            statusBar()->showMessage(tr("播放结束：共 %1 帧。").arg(trajectory.size()));
        } else {
            statusBar()->showMessage(tr("已经到达最后一帧。"));
        }
        return;
    }
    ApplyFrame(currentFrame + 1);
    UpdateDisplay();
    if (!playing) {
        statusBar()->showMessage(tr("手动前进到第 %1 / %2 帧。")
                                     .arg(currentFrame + 1)
                                     .arg(trajectory.size()));
        return;
    }
    if (currentFrame >= lastFrame) {
        // 最后一帧已经显示，立即停止定时器，避免再空转一个间隔。
        timer->stop();
        UpdateFrameControls();
        statusBar()->showMessage(tr("播放结束：共 %1 帧。").arg(trajectory.size()));
    }
}

void MainWindow::OnResetAnimation()
{
    if (trajectory.empty()) {
        statusBar()->showMessage(tr("没有可重置的轨迹：请先生成轨迹。"));
        return;
    }
    StopAnimation();
    ApplyFrame(0);
    UpdateDisplay();
    statusBar()->showMessage(tr("已回到轨迹起点：第 1 / %1 帧。").arg(trajectory.size()));
}

void MainWindow::OnSeekFrame(int frame)
{
    if (trajectory.empty()) {
        return;
    }
    StopAnimation();
    ApplyFrame(frame);
    UpdateDisplay();
    statusBar()->showMessage(tr("定位到第 %1 / %2 帧。")
                                 .arg(currentFrame + 1)
                                 .arg(trajectory.size()));
}
