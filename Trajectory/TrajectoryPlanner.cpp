#include "pch.h"
#include "TrajectoryPlanner.h"
#include <cmath>

TrajectoryPlanner::TrajectoryPlanner() = default;
TrajectoryPlanner::~TrajectoryPlanner() = default;
TrajectoryResult TrajectoryPlanner::PlanJoint(const RobotArm& /*arm*/, ArmAngleStruct target,
                                              double duration, double timeStep)
{
    if (!std::isfinite(target.q1) || !std::isfinite(target.q2)
        || !std::isfinite(duration) || !std::isfinite(timeStep)) return {Status::InvalidInput};
    if (duration <= 0.0 || timeStep <= 0.0) return {Status::InvalidParameter};
    // Implementation belongs to the trajectory workstream.
    return {Status::NotImplemented};
}
TrajectoryResult TrajectoryPlanner::PlanCartesian(const RobotArm& /*arm*/, Point2D target, bool /*elbowUp*/,
                                                  double duration, double timeStep)
{
    if (!std::isfinite(target.x) || !std::isfinite(target.y)
        || !std::isfinite(duration) || !std::isfinite(timeStep)) return {Status::InvalidInput};
    if (duration <= 0.0 || timeStep <= 0.0) return {Status::InvalidParameter};
    // Implementation belongs to the trajectory workstream.
    return {Status::NotImplemented};
}
