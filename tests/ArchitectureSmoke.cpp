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

        ArmAngleStruct result{11.0, 12.0};
        Require(InverseSolve({3.0, 4.0}, Point2D{4.0, 3.0}, true, result) == IKStatus::Success,
            "elbow-up inverse succeeds");
        Require(Near(result.q1, halfPi) && Near(result.q2, -halfPi), "elbow-up inverse angles");
        Require(InverseSolve({3.0, 4.0}, Point2D{3.0, 4.0}, false, result) == IKStatus::Success,
            "elbow-down inverse succeeds");
        Require(Near(result.q1, 0.0) && Near(result.q2, halfPi), "elbow-down inverse angles");
        Require(InverseSolve({3.0, 4.0}, ArmPosStruct{{0.0, 3.0}, {4.0, 3.0}}, result) == IKStatus::Success,
            "position inverse succeeds");
        Require(Near(result.q1, halfPi) && Near(result.q2, -halfPi), "position inverse relative angle");
        Require(InverseSolve({3.0, 4.0}, ArmPosStruct{{3.0, 0.0}, {3.0, 4.0}}, result) == IKStatus::Success,
            "position inverse supports opposite elbow");
        Require(Near(result.q1, 0.0) && Near(result.q2, halfPi), "position inverse opposite angles");
        Require(InverseSolve({3.0, 4.0}, ArmPosStruct{{-3.0, 0.0}, {-3.0, -4.0}}, result) == IKStatus::Success,
            "position inverse supports negative coordinates");
        Require(Near(std::abs(result.q1), 2.0 * halfPi) && Near(result.q2, halfPi), "relative angle wraps at pi");

        // Check both inverse overloads by reconstructing all positions in each quadrant.
        for (double q1 : {-2.8, -1.2, 0.4, 2.8}) {
            for (double q2 : {-1.2, -0.7, 0.7, 1.2}) {
                const ArmLengthStruct length{3.0, 4.0};
                const auto expected = ForwardSolve(length, {q1, q2});
                Require(InverseSolve(length, expected.L2, q2 < 0.0, result) == IKStatus::Success,
                    "target inverse succeeds across quadrants");
                auto reconstructed = ForwardSolve(length, result);
                Require(Near(reconstructed.L1.x, expected.L1.x) && Near(reconstructed.L1.y, expected.L1.y)
                    && Near(reconstructed.L2.x, expected.L2.x) && Near(reconstructed.L2.y, expected.L2.y),
                    "target inverse forward substitution preserves positions");
                Require(InverseSolve(length, expected, result) == IKStatus::Success,
                    "position inverse succeeds across quadrants");
                reconstructed = ForwardSolve(length, result);
                Require(Near(reconstructed.L1.x, expected.L1.x) && Near(reconstructed.L1.y, expected.L1.y)
                    && Near(reconstructed.L2.x, expected.L2.x) && Near(reconstructed.L2.y, expected.L2.y),
                    "position inverse forward substitution preserves positions");
            }
        }

        for (double scale : {1e-200, 1e200}) {
            Require(InverseSolve({3.0 * scale, 4.0 * scale}, Point2D{4.0 * scale, 3.0 * scale}, true, result)
                == IKStatus::Success, "target inverse supports small and large units");
            Require(Near(result.q1, halfPi) && Near(result.q2, -halfPi), "scaled target inverse angles");
            Require(InverseSolve({3.0 * scale, 4.0 * scale},
                ArmPosStruct{{0.0, 3.0 * scale}, {4.0 * scale, 3.0 * scale}}, result)
                == IKStatus::Success, "position inverse supports small and large units");
            Require(Near(result.q1, halfPi) && Near(result.q2, -halfPi), "scaled position inverse angles");
        }

        const double nan = std::numeric_limits<double>::quiet_NaN();
        const double inf = std::numeric_limits<double>::infinity();
        const struct { ArmLengthStruct length; Point2D target; IKStatus status; } failedTargets[] = {
            {{0.0, 4.0}, {3.0, 4.0}, IKStatus::InvalidLength},
            {{3.0, -4.0}, {3.0, 4.0}, IKStatus::InvalidLength},
            {{nan, 4.0}, {3.0, 4.0}, IKStatus::InvalidLength},
            {{3.0, inf}, {3.0, 4.0}, IKStatus::InvalidLength},
            {{3.0, 4.0}, {8.0, 0.0}, IKStatus::Unreachable},
            {{3.0, 4.0}, {0.5, 0.0}, IKStatus::Unreachable},
            {{3.0, 4.0}, {nan, 0.0}, IKStatus::Unreachable},
            {{3.0, 4.0}, {0.0, inf}, IKStatus::Unreachable},
            {{3.0, 4.0}, {7.0, 0.0}, IKStatus::SingularStraight},
            {{4.0, 3.0}, {1.0, 0.0}, IKStatus::SingularFolded},
            {{3.0, 3.0}, {0.0, 0.0}, IKStatus::SingularFolded}
        };
        for (const auto& test : failedTargets) {
            for (bool elbowUp : {false, true}) {
                result = {11.0, 12.0};
                Require(InverseSolve(test.length, test.target, elbowUp, result) == test.status, "target inverse status");
                Require(result.q1 == 11.0 && result.q2 == 12.0, "target inverse failure preserves output");
            }
        }
        const struct { ArmLengthStruct length; ArmPosStruct pos; IKStatus status; } failedPositions[] = {
            {{-3.0, 4.0}, {{3.0, 0.0}, {3.0, 4.0}}, IKStatus::InvalidLength},
            {{3.0, nan}, {{3.0, 0.0}, {3.0, 4.0}}, IKStatus::InvalidLength},
            {{3.0, 4.0}, {{2.0, 0.0}, {2.0, 4.0}}, IKStatus::Unreachable},
            {{3.0, 4.0}, {{3.0, 0.0}, {3.0, 5.0}}, IKStatus::Unreachable},
            {{3.0, 4.0}, {{nan, 0.0}, {3.0, 4.0}}, IKStatus::Unreachable},
            {{3.0, 4.0}, {{3.0, 0.0}, {inf, 4.0}}, IKStatus::Unreachable},
            {{3.0, 4.0}, {{3.0, 0.0}, {7.0, 0.0}}, IKStatus::SingularStraight},
            {{4.0, 3.0}, {{4.0, 0.0}, {1.0, 0.0}}, IKStatus::SingularFolded},
            {{3.0, 3.0}, {{3.0, 0.0}, {0.0, 0.0}}, IKStatus::SingularFolded}
        };
        for (const auto& test : failedPositions) {
            result = {11.0, 12.0};
            Require(InverseSolve(test.length, test.pos, result) == test.status, "position inverse status");
            Require(result.q1 == 11.0 && result.q2 == 12.0, "position inverse failure preserves output");
        }

        TrajectoryPlanner planner;
        TRAJECTORY trajectory;
        arm.SetPara({3.0, 4.0}, {halfPi, -halfPi});

        Require(planner.PlanJoint(arm, {0.0, 0.0}, 1.0, 0.3, trajectory), "joint plan succeeds");
        const double expectedTimes[] = {0.0, 0.3, 0.6, 0.9, 1.0};
        Require(trajectory.size() == 5, "joint sample count includes partial final interval");
        for (std::size_t i = 0; i < trajectory.size(); ++i) {
            const auto& point = trajectory[i];
            const double expectedAngle = halfPi * (1.0 - expectedTimes[i]);
            Require(Near(point.time, expectedTimes[i]), "joint sample time");
            Require(Near(point.angle.q1, expectedAngle) && Near(point.angle.q2, -expectedAngle),
                "joint angle interpolation");
            const auto position = ForwardSolve(arm.QueryLength(), point.angle);
            Require(Near(point.pos.L2.x, position.L2.x) && Near(point.pos.L2.y, position.L2.y),
                "joint positions agree with angles");
        }

        Require(planner.PlanCartesian(arm, {3.0, 4.0}, true, 1.0, 0.3, trajectory),
            "Cartesian plan succeeds");
        Require(trajectory.size() == 5, "Cartesian sample count");
        for (std::size_t i = 0; i < trajectory.size(); ++i) {
            const auto& point = trajectory[i];
            Require(Near(point.time, expectedTimes[i]), "Cartesian sample time");
            Require(Near(point.pos.L2.x, 4.0 - expectedTimes[i])
                && Near(point.pos.L2.y, 3.0 + expectedTimes[i]), "Cartesian straight line");
            Require(point.angle.q2 < 0.0, "Cartesian keeps the requested elbow branch");
        }

        for (double invalidTime : {0.0, -1.0, nan, inf}) {
            trajectory.resize(1);
            Require(!planner.PlanJoint(arm, {0.0, 0.0}, invalidTime, 0.1, trajectory)
                && trajectory.empty(), "invalid duration clears joint output");
            trajectory.resize(1);
            Require(!planner.PlanCartesian(arm, {3.0, 4.0}, true, 1.0, invalidTime, trajectory)
                && trajectory.empty(), "invalid step clears Cartesian output");
        }

        trajectory.resize(1);
        Require(!planner.PlanCartesian(arm, {8.0, 0.0}, true, 1.0, 0.3, trajectory)
            && trajectory.empty(), "unreachable target clears output");
        trajectory.resize(1);
        Require(!planner.PlanCartesian(arm, {-4.0, -3.0}, true, 1.0, 2.0, trajectory)
            && trajectory.empty(), "unreachable segment interior clears output");
        Require(Near(arm.QueryAngle().q1, halfPi) && Near(arm.QueryAngle().q2, -halfPi)
            && Near(arm.QueryPosL2().x, 4.0) && Near(arm.QueryPosL2().y, 3.0),
            "planning preserves the arm state");
        std::cout << "Architecture smoke checks passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
