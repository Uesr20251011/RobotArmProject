#include "RobotArm.h"
#include "Kinematics.h"
#include "TrajectoryPlanner.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
void Require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}
bool Near(double left, double right) { return std::abs(left - right) < 1e-9; }
struct ArmState { ArmLengthStruct length; ArmAngleStruct angle; ArmPosStruct pos; };
ArmState Capture(const RobotArm& arm) { return {arm.QueryLength(), arm.QueryAngle(), arm.QueryPos()}; }
void RequireUnchanged(const RobotArm& arm, const ArmState& before)
{
    const auto after = Capture(arm);
    Require(after.length.length1 == before.length.length1 && after.length.length2 == before.length.length2
        && after.angle.q1 == before.angle.q1 && after.angle.q2 == before.angle.q2
        && after.pos.L1.x == before.pos.L1.x && after.pos.L1.y == before.pos.L1.y
        && after.pos.L2.x == before.pos.L2.x && after.pos.L2.y == before.pos.L2.y,
        "failed operation must preserve the complete arm state");
}
}

int main()
{
    try {
        const double nan = std::numeric_limits<double>::quiet_NaN();
        const double inf = std::numeric_limits<double>::infinity();
        const double max = std::numeric_limits<double>::max();
        const FKResult emptyForward{};
        const IKResult emptyInverse{};
        const TrajectoryResult emptyTrajectory{};
        Require(emptyForward.status == Status::NotInitialized && emptyForward.pos.L1.x == 0.0
            && emptyForward.pos.L2.y == 0.0, "default forward result must be initialized");
        Require(emptyInverse.status == Status::NotInitialized && emptyInverse.angle.q1 == 0.0
            && emptyInverse.angle.q2 == 0.0, "default inverse result must be initialized");
        Require(emptyTrajectory.status == Status::NotInitialized && emptyTrajectory.trajectory.empty(),
            "default trajectory result must be initialized");

        RobotArm arm;
        Require(Near(arm.QueryPosL2().x, 200.0), "default arm remains initialized");
        Require(arm.SetPara({3.0, 4.0}, {0.0, 0.0}) == Status::Success, "set lengths and angles");
        Require(Near(arm.QueryLength().length2, 4.0) && Near(arm.QueryPosL1().x, 3.0)
            && Near(arm.QueryPosL2().x, 7.0), "state update across DLL");
        const double halfPi = std::acos(-1.0) / 2.0;
        Require(arm.SetPara(ArmAngleStruct{halfPi, -halfPi}) == Status::Success, "set angles");
        const auto forward = ForwardSolve(arm.QueryLength(), arm.QueryAngle());
        Require(forward.status == Status::Success, "forward solve succeeds");
        Require(Near(forward.pos.L2.x, 4.0) && Near(forward.pos.L2.y, 3.0), "forward result across DLL");
        Require(Near(arm.QueryPos().L2.y, forward.pos.L2.y), "shared geometry");
        const ArmPosStruct explicitPosition{{1.0, 2.0}, {3.0, 4.0}};
        Require(arm.SetPara({3.0, 4.0}, {0.0, 0.0}, explicitPosition) == Status::Success, "explicit state");
        const auto before = Capture(arm);
        Require(Near(before.pos.L2.y, 4.0), "explicit state preserves supplied positions");

        Require(arm.SetPara({0.0, 4.0}, {1.0, 2.0}) == Status::InvalidParameter, "reject zero length");
        RequireUnchanged(arm, before);
        Require(arm.SetPara({-3.0, 4.0}, {1.0, 2.0}, explicitPosition) == Status::InvalidParameter, "reject negative length");
        RequireUnchanged(arm, before);
        Require(arm.SetPara({nan, 4.0}, {1.0, 2.0}) == Status::InvalidInput, "reject nonfinite length");
        RequireUnchanged(arm, before);
        Require(arm.SetPara(ArmAngleStruct{inf, 1.0}) == Status::InvalidInput, "reject nonfinite angle");
        RequireUnchanged(arm, before);
        Require(arm.SetPara({3.0, 4.0}, {0.0, nan}, explicitPosition) == Status::InvalidInput, "reject explicit invalid angle");
        RequireUnchanged(arm, before);
        Require(arm.SetPara({3.0, 4.0}, {1.0, 2.0}, {{0.0, 0.0}, {inf, 0.0}}) == Status::InvalidInput,
            "reject nonfinite explicit position");
        RequireUnchanged(arm, before);
        Require(arm.SetPara({max, max}, {0.0, 0.0}) == Status::NumericalError, "detect computed position overflow");
        RequireUnchanged(arm, before);
        Require(arm.SetPara(ArmAngleStruct{max, max}) == Status::NumericalError, "detect angle sum overflow");
        RequireUnchanged(arm, before);

        Require(ForwardSolve({-1.0, 4.0}, {}).status == Status::InvalidParameter, "forward invalid parameter");
        const auto failedForward = ForwardSolve({3.0, 4.0}, {nan, 0.0});
        Require(failedForward.status == Status::InvalidInput && failedForward.pos.L2.x == 0.0,
            "forward failure carries status and default data");
        Require(ForwardSolve({max, max}, {}).status == Status::NumericalError, "forward overflow status");

        // Initialization-only checks: replace NotImplemented expectations when algorithms land.
        const auto inverseTarget = InverseSolve({3.0, 4.0}, Point2D{4.0, 3.0}, true);
        const auto inversePositions = InverseSolve({3.0, 4.0}, forward.pos);
        Require(inverseTarget.status == Status::NotImplemented && inverseTarget.angle.q1 == 0.0
            && inverseTarget.angle.q2 == 0.0, "inverse target scaffold has explicit initialized failure");
        Require(inversePositions.status == Status::NotImplemented && inversePositions.angle.q2 == 0.0,
            "inverse positions scaffold has explicit initialized failure");
        Require(InverseSolve({3.0, 4.0}, Point2D{nan, 0.0}, false).status == Status::InvalidInput, "inverse target invalid input");
        Require(InverseSolve({0.0, 4.0}, Point2D{}, true).status == Status::InvalidParameter, "inverse target invalid length");
        Require(InverseSolve({3.0, 4.0}, ArmPosStruct{{0.0, inf}, {0.0, 0.0}}).status == Status::InvalidInput,
            "inverse positions invalid input");
        Require(InverseSolve({3.0, -4.0}, forward.pos).status == Status::InvalidParameter, "inverse positions invalid length");

        TrajectoryPlanner planner;
        const auto joint = planner.PlanJoint(arm, {1.0, 1.0}, 1.0, 0.1);
        const auto cartesian = planner.PlanCartesian(arm, {4.0, 3.0}, true, 1.0, 0.1);
        Require(joint.status == Status::NotImplemented && joint.trajectory.empty(), "joint scaffold returns empty failure");
        Require(cartesian.status == Status::NotImplemented && cartesian.trajectory.empty(), "Cartesian scaffold returns empty failure");
        const auto invalidJoint = planner.PlanJoint(arm, {}, 0.0, 0.1);
        Require(invalidJoint.status == Status::InvalidParameter && invalidJoint.trajectory.empty(), "joint invalid duration");
        Require(planner.PlanJoint(arm, {nan, 0.0}, 1.0, 0.1).status == Status::InvalidInput, "joint invalid target");
        Require(planner.PlanJoint(arm, {}, inf, 0.1).status == Status::InvalidInput, "joint nonfinite time");
        const auto invalidCartesian = planner.PlanCartesian(arm, {}, true, 1.0, -0.1);
        Require(invalidCartesian.status == Status::InvalidParameter && invalidCartesian.trajectory.empty(), "Cartesian invalid step");
        Require(planner.PlanCartesian(arm, {inf, 0.0}, true, 1.0, 0.1).status == Status::InvalidInput, "Cartesian invalid target");
        Require(planner.PlanCartesian(arm, {}, true, 1.0, nan).status == Status::InvalidInput, "Cartesian nonfinite time");
        RequireUnchanged(arm, before);
        std::cout << "Architecture and status contract checks passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
