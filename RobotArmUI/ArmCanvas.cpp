#include "ArmCanvas.h"

#include <QFontMetrics>
#include <QPainter>
#include <QPolygonF>
#include <QStringList>
#include <QtMath>
#include <algorithm>
#include <cmath>

namespace {
constexpr int kOuterMargin = 22;
constexpr double kReachPadding = 1.10;
constexpr double kTargetGridSpacing = 62.0;
constexpr double kTwoPi = 6.28318530717958647692;

const QColor kBackgroundColor(0xf8, 0xfa, 0xfc);
const QColor kGridColor(0xe2, 0xe8, 0xf0);
const QColor kMajorGridColor(0xcb, 0xd5, 0xe1);
const QColor kAxisColor(0x94, 0xa3, 0xb8);
const QColor kBorderColor(0xcb, 0xd5, 0xe1);
const QColor kTextColor(0x33, 0x41, 0x55);
const QColor kMutedTextColor(0x64, 0x74, 0x8b);
const QColor kLink1Color(0x25, 0x63, 0xeb);
const QColor kLink2Color(0xf9, 0x73, 0x16);
const QColor kTrajectoryColor(0x0d, 0x94, 0x88);
const QColor kElbowPathColor(0x60, 0xa5, 0xfa);
const QColor kTargetColor(0xd9, 0x46, 0xef);
const QColor kEndEffectorColor(0xdc, 0x26, 0x26);
const QColor kJointColor(0x1e, 0x29, 0x3b);

QString FormatNumber(double value, int decimals)
{
    if (qAbs(value) < 5e-10) value = 0.0;
    return QString::number(value, 'f', decimals);
}
}

ArmCanvas::ArmCanvas(QWidget* parent) : QWidget(parent)
{
    setMinimumSize(420, 360);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAutoFillBackground(false);
    RefreshWorldExtent();
}

void ArmCanvas::SetArmState(ArmLengthStruct length, ArmAngleStruct angle, ArmPosStruct pos)
{
    armLength = length;
    armAngle = angle;
    armPos = pos;
    RefreshWorldExtent();
    update();
}

void ArmCanvas::SetTrajectory(const TRAJECTORY& plannedTrajectory)
{
    trajectory = plannedTrajectory;
    if (currentFrame >= static_cast<int>(trajectory.size())) {
        currentFrame = trajectory.empty() ? -1 : static_cast<int>(trajectory.size()) - 1;
    }
    trajectoryExtent = 0.0;
    for (const auto& point : trajectory) {
        trajectoryExtent = (std::max)(trajectoryExtent,
                                      (std::max)((std::max)(qAbs(point.pos.L1.x), qAbs(point.pos.L1.y)),
                                                 (std::max)(qAbs(point.pos.L2.x), qAbs(point.pos.L2.y))));
    }
    RefreshWorldExtent();
    update();
}

void ArmCanvas::SetCurrentFrame(int frame)
{
    currentFrame = frame;
    update();
}

void ArmCanvas::SetTargetMarker(Point2D marker, bool visible)
{
    target = marker;
    targetVisible = visible;
    RefreshWorldExtent();
    update();
}

QSize ArmCanvas::sizeHint() const
{
    return {860, 640};
}

void ArmCanvas::RefreshWorldExtent()
{
    // 只在数据变化时调用，因此这里不遍历整条轨迹（轨迹范围已在 SetTrajectory 中缓存）。
    double extent = (armLength.length1 + armLength.length2) * kReachPadding;
    const auto consider = [&extent](Point2D point) {
        extent = (std::max)(extent, (std::max)(qAbs(point.x), qAbs(point.y)));
    };
    consider(armPos.L1);
    consider(armPos.L2);
    if (targetVisible) {
        consider(target);
    }
    extent = (std::max)(extent, trajectoryExtent);
    if (!std::isfinite(extent) || extent <= 0.0) {
        extent = 100.0;
    }
    cachedExtent = extent;
}

ArmCanvas::View ArmCanvas::ComputeView() const
{
    View view;
    view.extent = cachedExtent;
    const double usableWidth = (std::max)(1.0, static_cast<double>(width()) - 2.0 * kOuterMargin);
    const double usableHeight = (std::max)(1.0, static_cast<double>(height()) - 2.0 * kOuterMargin);
    view.scale = (std::min)(usableWidth, usableHeight) / (2.0 * view.extent);
    if (!std::isfinite(view.scale) || view.scale <= 0.0) {
        view.scale = 1.0;
    }
    view.origin = QPointF(width() / 2.0, height() / 2.0);
    return view;
}

QPointF ArmCanvas::ToScreen(const View& view, Point2D point) const
{
    return {view.origin.x() + point.x * view.scale, view.origin.y() - point.y * view.scale};
}

double ArmCanvas::PickGridStep(double scale)
{
    if (!std::isfinite(scale) || scale <= 0.0) {
        return 1.0;
    }
    const double rawStep = kTargetGridSpacing / scale;
    double magnitude = std::pow(10.0, std::floor(std::log10(rawStep)));
    static const double kFactors[] = {1.0, 2.0, 5.0, 10.0};
    for (const double factor : kFactors) {
        const double step = magnitude * factor;
        if (step >= rawStep) {
            return step;
        }
    }
    return magnitude * 10.0;
}

QString ArmCanvas::FormatTick(double value, double step)
{
    if (!std::isfinite(value) || !std::isfinite(step) || step <= 0.0) {
        return QStringLiteral("?");
    }
    if (qAbs(value) < step * 1e-6) {
        return QStringLiteral("0");
    }
    const int decimals = (std::max)(0, static_cast<int>(std::ceil(-std::log10(step))));
    return QString::number(value, 'f', (std::min)(decimals, 6));
}

void ArmCanvas::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), kBackgroundColor);
    if (width() < 120 || height() < 120) {
        return;
    }
    const View view = ComputeView();
    DrawGrid(painter, view);
    DrawWorkspace(painter, view);
    DrawTrajectory(painter, view);
    DrawTarget(painter, view);
    DrawRobot(painter, view);
    DrawAngleArcs(painter, view);
    DrawOverlay(painter, view);

    painter.setPen(QPen(kBorderColor, 1.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
}

void ArmCanvas::DrawGrid(QPainter& painter, const View& view) const
{
    const double step = PickGridStep(view.scale);
    const double left = -(view.origin.x()) / view.scale;
    const double right = (width() - view.origin.x()) / view.scale;
    const double top = view.origin.y() / view.scale;
    const double bottom = -(height() - view.origin.y()) / view.scale;
    if (!std::isfinite(step) || step <= 0.0 || !std::isfinite(left) || !std::isfinite(right)
        || !std::isfinite(top) || !std::isfinite(bottom)) {
        return;
    }

    QFont labelFont = font();
    labelFont.setPointSizeF(8.0);
    painter.setFont(labelFont);

    // 每 5 格画一条更明显的辅助线，避免整片网格过于抢眼。
    const int firstXIndex = static_cast<int>(std::floor(left / step));
    const int lastXIndex = static_cast<int>(std::ceil(right / step));
    for (int index = firstXIndex; index <= lastXIndex && index - firstXIndex < 400; ++index) {
        const double worldX = index * step;
        const double screenX = view.origin.x() + worldX * view.scale;
        const bool major = index % 5 == 0;
        painter.setPen(QPen(major ? kMajorGridColor : kGridColor, 1.0));
        painter.drawLine(QPointF(screenX, 0.0), QPointF(screenX, height()));
        if (index != 0) {
            painter.setPen(QPen(kMutedTextColor, 1.0));
            painter.drawText(QPointF(screenX + 3.0, height() - 7.0), FormatTick(worldX, step));
        }
    }
    const int firstYIndex = static_cast<int>(std::floor(bottom / step));
    const int lastYIndex = static_cast<int>(std::ceil(top / step));
    for (int index = firstYIndex; index <= lastYIndex && index - firstYIndex < 400; ++index) {
        const double worldY = index * step;
        const double screenY = view.origin.y() - worldY * view.scale;
        const bool major = index % 5 == 0;
        painter.setPen(QPen(major ? kMajorGridColor : kGridColor, 1.0));
        painter.drawLine(QPointF(0.0, screenY), QPointF(width(), screenY));
        if (index != 0) {
            painter.setPen(QPen(kMutedTextColor, 1.0));
            painter.drawText(QPointF(5.0, screenY - 4.0), FormatTick(worldY, step));
        }
    }

    // x / y 轴。
    painter.setPen(QPen(kAxisColor, 1.4));
    if (view.origin.y() >= 0.0 && view.origin.y() <= height()) {
        painter.drawLine(QPointF(0.0, view.origin.y()), QPointF(width(), view.origin.y()));
    }
    if (view.origin.x() >= 0.0 && view.origin.x() <= width()) {
        painter.drawLine(QPointF(view.origin.x(), 0.0), QPointF(view.origin.x(), height()));
    }
}

void ArmCanvas::DrawWorkspace(QPainter& painter, const View& view) const
{
    const double reach = (armLength.length1 + armLength.length2) * view.scale;
    if (!std::isfinite(reach) || reach <= 0.0) {
        return;
    }
    painter.setPen(QPen(QColor(0x94, 0xa3, 0xb8, 150), 1.0, Qt::DashLine));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(view.origin, reach, reach);
}

void ArmCanvas::DrawTrajectory(QPainter& painter, const View& view) const
{
    if (trajectory.size() < 2) {
        return;
    }
    QPolygonF elbowPath;
    QPolygonF endPath;
    elbowPath.reserve(static_cast<int>(trajectory.size()));
    endPath.reserve(static_cast<int>(trajectory.size()));
    for (const auto& point : trajectory) {
        elbowPath.append(ToScreen(view, point.pos.L1));
        endPath.append(ToScreen(view, point.pos.L2));
    }

    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(kElbowPathColor, 1.1, Qt::DashLine));
    painter.drawPolyline(elbowPath);
    painter.setPen(QPen(kTrajectoryColor, 2.0));
    painter.drawPolyline(endPath);

    // 起点与终点标记。
    const QPointF start = endPath.first();
    const QPointF finish = endPath.last();
    painter.setPen(QPen(kTrajectoryColor, 1.6));
    painter.setBrush(QColor(255, 255, 255));
    painter.drawEllipse(start, 4.5, 4.5);
    painter.setBrush(kTrajectoryColor);
    painter.drawRect(QRectF(finish.x() - 9.0, finish.y() - 9.0, 18.0, 18.0));

    if (currentFrame >= 0 && currentFrame < static_cast<int>(trajectory.size())) {
        const QPointF current = ToScreen(view, trajectory[static_cast<size_t>(currentFrame)].pos.L2);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(13, 148, 136, 55));
        painter.drawEllipse(current, 14.0, 14.0);
        painter.setBrush(kTrajectoryColor);
        painter.drawEllipse(current, 5.0, 5.0);
    }
}

void ArmCanvas::DrawTarget(QPainter& painter, const View& view) const
{
    if (!targetVisible || !std::isfinite(target.x) || !std::isfinite(target.y)) {
        return;
    }
    const QPointF point = ToScreen(view, target);
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(kTargetColor, 1.6));
    painter.drawEllipse(point, 9.0, 9.0);
    painter.drawLine(QPointF(point.x() - 15.0, point.y()), QPointF(point.x() - 4.0, point.y()));
    painter.drawLine(QPointF(point.x() + 4.0, point.y()), QPointF(point.x() + 15.0, point.y()));
    painter.drawLine(QPointF(point.x(), point.y() - 15.0), QPointF(point.x(), point.y() - 4.0));
    painter.drawLine(QPointF(point.x(), point.y() + 4.0), QPointF(point.x(), point.y() + 15.0));

    QFont targetFont = font();
    targetFont.setPointSizeF(8.0);
    painter.setFont(targetFont);
    painter.setPen(kTargetColor);
    painter.drawText(QPointF(point.x() + 13.0, point.y() + 20.0),
                     tr("目标 (%1, %2)")
                         .arg(FormatNumber(target.x, 2))
                         .arg(FormatNumber(target.y, 2)));
}

void ArmCanvas::DrawRobot(QPainter& painter, const View& view) const
{
    const QPointF base = ToScreen(view, Point2D{0.0, 0.0});
    const QPointF elbow = ToScreen(view, armPos.L1);
    const QPointF end = ToScreen(view, armPos.L2);

    painter.setPen(QPen(kLink1Color, 9.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawLine(base, elbow);
    painter.setPen(QPen(kLink2Color, 7.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawLine(elbow, end);

    const auto drawJoint = [&painter](QPointF center, double radius, const QColor& color) {
        painter.setPen(QPen(Qt::white, 2.4));
        painter.setBrush(color);
        painter.drawEllipse(center, radius, radius);
    };
    drawJoint(base, 10.0, kJointColor);
    drawJoint(elbow, 7.5, kJointColor);

    // 末端执行器：红色圆点 + 白色描边。
    painter.setPen(QPen(Qt::white, 2.2));
    painter.setBrush(kEndEffectorColor);
    painter.drawEllipse(end, 6.5, 6.5);
}

void ArmCanvas::DrawAngleArcs(QPainter& painter, const View& view) const
{
    const double q1 = std::remainder(armAngle.q1, kTwoPi);
    const double q2 = std::remainder(armAngle.q2, kTwoPi);
    const double q1Degrees = qRadiansToDegrees(q1);
    const double q2Degrees = qRadiansToDegrees(q2);
    if (qAbs(q1Degrees) < 0.05 && qAbs(q2Degrees) < 0.05) {
        return;
    }

    QFont arcFont = font();
    arcFont.setPointSizeF(8.0);
    painter.setFont(arcFont);
    painter.setBrush(Qt::NoBrush);

    const QPointF base = ToScreen(view, Point2D{0.0, 0.0});
    const QPointF elbow = ToScreen(view, armPos.L1);

    // q1：从 x 轴正向转到第一根连杆。
    const double radius1 = 46.0;
    if (qAbs(q1Degrees) > 0.05) {
        painter.setPen(QPen(QColor(37, 99, 235, 170), 1.3));
        painter.drawArc(QRectF(base.x() - radius1, base.y() - radius1, 2.0 * radius1, 2.0 * radius1),
                        0, static_cast<int>(q1Degrees * 16.0));
        const double mid = q1 * 0.5;
        const QPointF label(base.x() + (radius1 + 10.0) * std::cos(mid),
                            base.y() - (radius1 + 10.0) * std::sin(mid));
        painter.setPen(kLink1Color);
        painter.drawText(label, tr("q1"));
    }

    // q2：从第一根连杆方向转到第二根连杆方向。
    const double radius2 = 34.0;
    if (qAbs(q2Degrees) > 0.05) {
        painter.setPen(QPen(QColor(249, 115, 22, 170), 1.3));
        painter.drawArc(QRectF(elbow.x() - radius2, elbow.y() - radius2, 2.0 * radius2, 2.0 * radius2),
                        static_cast<int>(q1Degrees * 16.0), static_cast<int>(q2Degrees * 16.0));
        const double mid = q1 + q2 * 0.5;
        const QPointF label(elbow.x() + (radius2 + 10.0) * std::cos(mid),
                            elbow.y() - (radius2 + 10.0) * std::sin(mid));
        painter.setPen(kLink2Color);
        painter.drawText(label, tr("q2"));
    }
}

void ArmCanvas::DrawOverlay(QPainter& painter, const View& view) const
{
    QFont titleFont = font();
    titleFont.setPointSizeF(9.0);
    titleFont.setBold(true);
    QFont infoFont = font();
    infoFont.setPointSizeF(8.5);
    painter.setFont(titleFont);
    const QFontMetrics titleMetrics = painter.fontMetrics();
    painter.setFont(infoFont);
    const QFontMetrics infoMetrics = painter.fontMetrics();

    const QString title = tr("二连杆机械臂：L1 = %1，L2 = %2")
                              .arg(FormatNumber(armLength.length1, 3))
                              .arg(FormatNumber(armLength.length2, 3));
    const QString detail =
        tr("q1 = %1°，q2 = %2°    末端 (%3, %4)")
            .arg(FormatNumber(qRadiansToDegrees(armAngle.q1), 2))
            .arg(FormatNumber(qRadiansToDegrees(armAngle.q2), 2))
            .arg(FormatNumber(armPos.L2.x, 3))
            .arg(FormatNumber(armPos.L2.y, 3));
    const QString helper = tr("网格 1 格 = %1，最大可达半径 = %2")
                               .arg(FormatNumber(PickGridStep(view.scale), 4))
                               .arg(FormatNumber(armLength.length1 + armLength.length2, 3));

    // 半透明白色底板，保证文字在网格和小尺寸窗口下仍然清晰。
    const int textWidth =
        (std::max)(titleMetrics.horizontalAdvance(title),
                   (std::max)(infoMetrics.horizontalAdvance(detail), infoMetrics.horizontalAdvance(helper)));
    const QRectF panel(8.0, 6.0, textWidth + 20.0,
                       titleMetrics.height() + 2.0 * infoMetrics.height() + 16.0);
    painter.setPen(QPen(QColor(203, 213, 225, 220), 1.0));
    painter.setBrush(QColor(255, 255, 255, 225));
    painter.drawRoundedRect(panel, 6.0, 6.0);

    const double textLeft = panel.left() + 10.0;
    double textTop = panel.top() + titleMetrics.ascent() + 4.0;
    painter.setFont(titleFont);
    painter.setPen(kTextColor);
    painter.drawText(QPointF(textLeft, textTop), title);
    textTop += infoMetrics.height();
    painter.setFont(infoFont);
    painter.setPen(kMutedTextColor);
    painter.drawText(QPointF(textLeft, textTop), detail);
    textTop += infoMetrics.height();
    painter.drawText(QPointF(textLeft, textTop), helper);

    // 右上角图例，同样加底板。
    painter.setFont(infoFont);
    const QFontMetrics legendMetrics = painter.fontMetrics();
    static const QStringList kLegendTitles = {QObject::tr("连杆 1"), QObject::tr("连杆 2"),
                                              QObject::tr("末端轨迹"), QObject::tr("目标点")};
    const QColor legendColors[] = {kLink1Color, kLink2Color, kTrajectoryColor, kTargetColor};
    int legendTextWidth = 0;
    for (const QString& legendTitle : kLegendTitles) {
        legendTextWidth = (std::max)(legendTextWidth, legendMetrics.horizontalAdvance(legendTitle));
    }
    const int legendRow = legendMetrics.height() + 3;
    const QRectF legendPanel(width() - (legendTextWidth + 58.0) - 8.0, 6.0,
                             legendTextWidth + 58.0, legendRow * 4 + 10.0);
    painter.setPen(QPen(QColor(203, 213, 225, 220), 1.0));
    painter.setBrush(QColor(255, 255, 255, 225));
    painter.drawRoundedRect(legendPanel, 6.0, 6.0);
    double legendTop = legendPanel.top() + 5.0 + legendMetrics.ascent();
    for (int index = 0; index < kLegendTitles.size(); ++index) {
        painter.setPen(QPen(legendColors[index], 3.0, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPointF(legendPanel.left() + 10.0, legendTop - legendMetrics.ascent() * 0.35),
                         QPointF(legendPanel.left() + 30.0, legendTop - legendMetrics.ascent() * 0.35));
        painter.setPen(kTextColor);
        painter.drawText(QPointF(legendPanel.left() + 36.0, legendTop), kLegendTitles[index]);
        legendTop += legendRow;
    }
}
