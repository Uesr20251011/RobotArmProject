#include "pch.h"
#include "TrajectoryPlanner.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr double FullTurn = 6.283185307179586;

double Lerp(double start, double target, double progress)
{
    return (1.0 - progress) * start + progress * target;
}

bool AddSampleTimes(double duration, double timeStep, TRAJECTORY& trajectory)
{
    if (!std::isfinite(duration) || duration <= 0.0
        || !std::isfinite(timeStep) || timeStep <= 0.0) {
        return false;
    }

    const double count = std::ceil(duration / timeStep);
    if (count >= static_cast<double>(trajectory.max_size())) return false;

    const auto intervals = static_cast<std::size_t>((std::max)(1.0, count));
    trajectory.reserve(intervals + 1);
    for (std::size_t i = 0; i < intervals; ++i) {
        const double time = i * timeStep;
        if (time >= duration) break;
        trajectory.push_back({time, {}, {}});
    }
    trajectory.push_back({duration, {}, {}});
    return true;
}

// Check the closest point to the origin if it lies between samples.
bool IsInteriorReachable(ArmLengthStruct length, Point2D start, Point2D target, bool elbowUp)
{
    const double scale = (std::max)({std::abs(start.x), std::abs(start.y),
                                     std::abs(target.x), std::abs(target.y)});
    if (scale == 0.0) return false;

    const double x = start.x / scale;
    const double y = start.y / scale;
    const double dx = target.x / scale - x;
    const double dy = target.y / scale - y;
    const double projection = -(x * dx + y * dy);
    const double squaredLength = dx * dx + dy * dy;
    if (projection <= 0.0 || projection >= squaredLength) return true;

    const double progress = projection / squaredLength;
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
    if (!AddSampleTimes(duration, timeStep, result)) return false;

    const auto length = arm.QueryLength();
    const auto start = arm.QueryAngle();
    for (auto& sample : result) {
        const double progress = sample.time / duration;
        sample.angle = {Lerp(start.q1, target.q1, progress), Lerp(start.q2, target.q2, progress)};
        sample.pos = ForwardSolve(length, sample.angle);
    }
    return true;
}

bool TrajectoryPlanner::PlanCartesian(const RobotArm& arm, Point2D target, bool elbowUp,
                                      double duration, double timeStep, TRAJECTORY& result)
{
    result.clear();
    const auto length = arm.QueryLength();
    auto previous = arm.QueryAngle();
    const auto start = ForwardSolve(length, previous).L2;

    if ((std::sin(previous.q2) < 0.0) != elbowUp
        || !IsInteriorReachable(length, start, target, elbowUp)
        || !AddSampleTimes(duration, timeStep, result)) {
        return false;
    }

    for (auto& sample : result) {
        const double progress = sample.time / duration;
        const Point2D point{Lerp(start.x, target.x, progress), Lerp(start.y, target.y, progress)};
        ArmAngleStruct angle{};
        if (InverseSolve(length, point, elbowUp, angle) != IKStatus::Success) {
            result.clear();
            return false;
        }

        // Keep the initial angles and choose the nearest equivalent angles on later frames.
        if (sample.time != 0.0) {
            previous.q1 += std::remainder(angle.q1 - previous.q1, FullTurn);
            previous.q2 += std::remainder(angle.q2 - previous.q2, FullTurn);
        }
        sample.angle = previous;
        sample.pos = ForwardSolve(length, sample.angle);
    }
    return true;
}
