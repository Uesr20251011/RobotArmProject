#pragma once
#include <vector>
#include "RobotArm.h"
#include "Kinematics.h"
#include "TrajectoryExport.h"

struct TrajectoryPoint {
    double time;
    ArmAngleStruct angle;
    ArmPosStruct pos;
};
using TRAJECTORY = std::vector<TrajectoryPoint>;

class TRAJECTORY_API TrajectoryPlanner {
public:
    TrajectoryPlanner();
    ~TrajectoryPlanner();
    // Clear result on failure, never modify arm. Scaffold always reports failure.
    bool PlanJoint(const RobotArm& arm, ArmAngleStruct target,
                   double duration, double timeStep, TRAJECTORY& result);
    bool PlanCartesian(const RobotArm& arm, Point2D target, bool elbowUp,
                       double duration, double timeStep, TRAJECTORY& result);
};
