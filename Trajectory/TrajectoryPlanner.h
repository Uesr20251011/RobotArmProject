#pragma once
#include <vector>
#include "RobotArm.h"
#include "Kinematics.h"
#include "TrajectoryExport.h"

struct TrajectoryPoint {
    double time{};
    ArmAngleStruct angle{};
    ArmPosStruct pos{};
};
using TRAJECTORY = std::vector<TrajectoryPoint>;

struct TrajectoryResult {
    Status status{Status::NotInitialized};
    TRAJECTORY trajectory{};
};

class TRAJECTORY_API TrajectoryPlanner {
public:
    TrajectoryPlanner();
    ~TrajectoryPlanner();
    // Failure returns an empty trajectory and never modifies arm.
    [[nodiscard]] TrajectoryResult PlanJoint(const RobotArm& arm, ArmAngleStruct target,
                                             double duration, double timeStep);
    [[nodiscard]] TrajectoryResult PlanCartesian(const RobotArm& arm, Point2D target, bool elbowUp,
                                                 double duration, double timeStep);
};
