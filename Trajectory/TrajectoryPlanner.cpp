#include "pch.h"
#include "TrajectoryPlanner.h"
#include <algorithm>
#include <cmath>
#include <utility>

namespace {
constexpr double MaxIntervals = 20000;
constexpr double FullTurn = 6.283185307179586;

double Lerp(double start, double target, double progress)
{
    return (1.0 - progress) * start + progress * target;
}

bool IsFinite(Point2D point)
{
    return std::isfinite(point.x) && std::isfinite(point.y);
}

template <typename AngleAt>
bool BuildTrajectory(ArmLengthStruct length, double duration, double timeStep,
                     TRAJECTORY& result, AngleAt angleAt)
{
    if (!std::isfinite(length.length1) || length.length1 <= 0.0
        || !std::isfinite(length.length2) || length.length2 <= 0.0
        || !std::isfinite(duration) || duration <= 0.0
        || !std::isfinite(timeStep) || timeStep <= 0.0) {
        return false;
    }

    const double intervalCount = (std::max)(1.0, std::ceil(duration / timeStep));
    if (intervalCount > MaxIntervals) return false;
    const auto intervals = static_cast<std::size_t>(intervalCount);

    TRAJECTORY samples;
    samples.reserve(intervals + 1);
    for (std::size_t index = 0; index <= intervals; ++index) {
        const double time = index == intervals
            ? duration : (std::min)(duration, index * timeStep);
        ArmAngleStruct angle{};
        if (!angleAt(time / duration, angle)
            || !std::isfinite(angle.q1) || !std::isfinite(angle.q2)) {
            return false;
        }
        const auto position = ForwardSolve(length, angle);
        if (!IsFinite(position.L1) || !IsFinite(position.L2)) return false;
        samples.push_back({time, angle, position});
        if (time == duration) break;
    }

    result = std::move(samples);
    return true;
}

// The reachable workspace is an annulus. Check the nearest point to the origin
// when it lies inside the segment, even if sampling skips that point.
bool IsInteriorReachable(ArmLengthStruct length, Point2D start, Point2D target, bool elbowUp)
{
    const double scale = (std::max)({std::abs(start.x), std::abs(start.y),
                                     std::abs(target.x), std::abs(target.y)});
    if (scale == 0.0) return false;

    const double x = start.x / scale;
    const double y = start.y / scale;
    const double dx = target.x / scale - x;
    const double dy = target.y / scale - y;
    const double squaredLength = dx * dx + dy * dy;
    if (squaredLength == 0.0) return true;

    const double progress = -(x * dx + y * dy) / squaredLength;
    if (progress <= 0.0 || progress >= 1.0) return true;

    const Point2D nearest{Lerp(start.x, target.x, progress), Lerp(start.y, target.y, progress)};
    ArmAngleStruct angle{};
    return InverseSolve(length, nearest, elbowUp, angle) == IKStatus::Success;
}
}

TrajectoryPlanner::TrajectoryPlanner() = default;
TrajectoryPlanner::~TrajectoryPlanner() = default;

bool TrajectoryPlanner::PlanJoint(const RobotArm& arm, ArmAngleStruct target,
                                  double duration, double timeStep, TRAJECTORY& result)
{
    result.clear();
    const auto start = arm.QueryAngle();
    return BuildTrajectory(arm.QueryLength(), duration, timeStep, result,
        [start, target](double progress, ArmAngleStruct& angle) {
            angle = {Lerp(start.q1, target.q1, progress), Lerp(start.q2, target.q2, progress)};
            return true;
        });
}

bool TrajectoryPlanner::PlanCartesian(const RobotArm& arm, Point2D target, bool elbowUp,
                                      double duration, double timeStep, TRAJECTORY& result)
{
    result.clear();
    const auto length = arm.QueryLength();
    const auto startAngle = arm.QueryAngle();
    const auto start = ForwardSolve(length, startAngle).L2;

    if (!IsFinite(start) || !IsFinite(target) || (std::sin(startAngle.q2) < 0.0) != elbowUp
        || !IsInteriorReachable(length, start, target, elbowUp)) {
        return false;
    }

    auto previous = startAngle;
    return BuildTrajectory(length, duration, timeStep, result,
        [&](double progress, ArmAngleStruct& angle) {
            const Point2D point{Lerp(start.x, target.x, progress), Lerp(start.y, target.y, progress)};
            if (InverseSolve(length, point, elbowUp, angle) != IKStatus::Success) return false;

            // Keep the starting pose and use equivalent angles near the preceding frame.
            if (progress == 0.0) {
                angle = startAngle;
            } else {
                angle.q1 = previous.q1 + std::remainder(angle.q1 - previous.q1, FullTurn);
                angle.q2 = previous.q2 + std::remainder(angle.q2 - previous.q2, FullTurn);
            }
            previous = angle;
            return true;
        });
}
