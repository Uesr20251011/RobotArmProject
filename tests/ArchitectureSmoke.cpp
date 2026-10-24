#include "RobotArm.h"
#include "Kinematics.h"
#include "TrajectoryPlanner.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void Require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}
bool Near(double left, double right) { return std::abs(left - right) < 1e-9; }
}

int main()
{
    try {
        RobotArm arm;
        Require(Near(arm.QueryPosL2().x, 200.0), "default arm state");
        arm.SetPara({3.0, 4.0}, {0.0, 0.0});
        Require(Near(arm.QueryLength().length2, 4.0), "length query across DLL");
        Require(Near(arm.QueryPosL1().x, 3.0), "elbow update");
        Require(Near(arm.QueryPosL2().x, 7.0), "end effector update");
        const double halfPi = std::acos(-1.0) / 2.0;
        arm.SetPara(ArmAngleStruct{halfPi, -halfPi});
        Require(Near(arm.QueryAngle().q1, halfPi), "angle query");
        const auto forward = ForwardSolve(arm.QueryLength(), arm.QueryAngle());
        Require(Near(forward.L2.x, 4.0) && Near(forward.L2.y, 3.0), "Common to Kinematics linkage");
        Require(Near(arm.QueryPos().L2.y, forward.L2.y), "shared geometry contract");
        const ArmPosStruct explicitPosition{{1.0, 2.0}, {3.0, 4.0}};
        arm.SetPara({3.0, 4.0}, {0.0, 0.0}, explicitPosition);
        Require(Near(arm.QueryPosL2().y, 4.0), "explicit state overload");

        // Initialization-only assertions. Replace with algorithm tests as implementations land.
        ArmAngleStruct result{11.0, 12.0};
        Require(!InverseSolve({3.0, 4.0}, Point2D{4.0, 3.0}, true, result), "inverse scaffold reports failure");
        Require(!InverseSolve({3.0, 4.0}, forward, result), "position inverse scaffold reports failure");
        Require(result.q1 == 11.0 && result.q2 == 12.0, "inverse failure preserves output");
        TrajectoryPlanner planner;
        TRAJECTORY trajectory(1);
        Require(!planner.PlanJoint(arm, {1.0, 1.0}, 1.0, 0.1, trajectory), "joint scaffold reports failure");
        Require(trajectory.empty(), "joint failure clears trajectory");
        trajectory.resize(1);
        Require(!planner.PlanCartesian(arm, {4.0, 3.0}, true, 1.0, 0.1, trajectory), "Cartesian scaffold reports failure");
        Require(trajectory.empty(), "Cartesian failure clears trajectory");
        Require(Near(arm.QueryPosL2().y, 4.0), "planning must not mutate arm");
        std::cout << "Architecture smoke checks passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
