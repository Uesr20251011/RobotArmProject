#include "pch.h"
#include "TrajectoryPlanner.h"

TrajectoryPlanner::TrajectoryPlanner() = default;
TrajectoryPlanner::~TrajectoryPlanner() = default;
bool TrajectoryPlanner::PlanJoint(const RobotArm& /*arm*/, ArmAngleStruct /*target*/,
                                  double /*duration*/, double /*timeStep*/, TRAJECTORY& result)
{
    // Implementation belongs to the trajectory workstream.
    result.clear();
    return false;
}
bool TrajectoryPlanner::PlanCartesian(const RobotArm& /*arm*/, Point2D /*target*/, bool /*elbowUp*/,
                                      double /*duration*/, double /*timeStep*/, TRAJECTORY& result)
{
    // Implementation belongs to the trajectory workstream.
    result.clear();
    return false;
}
