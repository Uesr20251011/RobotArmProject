#include "pch.h"
#include "Kinematics.h"
#include <algorithm>
#include <cmath>

ArmPosStruct ForwardSolve(ArmLengthStruct armL, ArmAngleStruct armQ)
{
    ArmPosStruct armP;
    armP.L1 = { armL.length1 * std::cos(armQ.q1), armL.length1 * std::sin(armQ.q1) };
    const double endAngle = armQ.q1 + armQ.q2;
    armP.L2 = { armP.L1.x + armL.length2 * std::cos(endAngle),
               armP.L1.y + armL.length2 * std::sin(endAngle) };
    return armP;
}

IKStatus InverseSolve(ArmLengthStruct armL, Point2D armQ,
                      bool elbowUp, ArmAngleStruct& result) // elbowUp = 1 肘部在末端与原点的连线上方
{
    double L1 = armL.length1;
    double L2 = armL.length2;

    double x = armQ.x;
    double y = armQ.y;

    if (!std::isfinite(L1) || !std::isfinite(L2) || L1 <= 0 || L2 <= 0)
        return IKStatus::InvalidLength;
    if (!std::isfinite(x) || !std::isfinite(y)) return IKStatus::Unreachable;

    // Normalize before products so changing length units cannot overflow or underflow them.
    const double scale = (std::max)({L1, L2, std::abs(x), std::abs(y)});
    L1 /= scale;
    L2 /= scale;
    x /= scale;
    y /= scale;

    double r2 = x * x + y * y;
    double cos2 = (r2 - L1 * L1 - L2 * L2) / (2.0 * L1 * L2);

    if (!std::isfinite(cos2) || cos2 < -1.0 - 1e-9 || cos2 > 1.0 + 1e-9)
        return IKStatus::Unreachable;
    cos2 = std::clamp(cos2, -1.0, 1.0);
    if (cos2 >= 1.0 - 1e-9) return IKStatus::SingularStraight;
    if (cos2 <= -1.0 + 1e-9) return IKStatus::SingularFolded;
    double theta2 = std::acos(cos2);

    if (elbowUp)
        theta2 = -theta2;

    double theta1 = std::atan2(y, x)
        - std::atan2(L2 * std::sin(theta2), L1 + L2 * std::cos(theta2));

    result.q1 = theta1;
    result.q2 = theta2;

    return IKStatus::Success;
}

IKStatus InverseSolve(ArmLengthStruct armL, ArmPosStruct armP, ArmAngleStruct& result)
{
    double L1 = armL.length1;
    double L2 = armL.length2;

    if (!std::isfinite(L1) || !std::isfinite(L2) || L1 <= 0 || L2 <= 0)
        return IKStatus::InvalidLength;
    if (!std::isfinite(armP.L1.x) || !std::isfinite(armP.L1.y)
        || !std::isfinite(armP.L2.x) || !std::isfinite(armP.L2.y))
        return IKStatus::Unreachable;

    double dx = armP.L2.x - armP.L1.x;
    double dy = armP.L2.y - armP.L1.y;
    double distance1 = std::hypot(armP.L1.x, armP.L1.y);
    double distance2 = std::hypot(dx, dy);

    // The supplied elbow and end point must agree with both link lengths.
    if (!std::isfinite(distance1) || !std::isfinite(distance2)
        || std::abs(distance1 / L1 - 1.0) > 1e-9
        || std::abs(distance2 / L2 - 1.0) > 1e-9)
        return IKStatus::Unreachable;

    double theta1 = std::atan2(armP.L1.y, armP.L1.x);
    double theta2 = std::atan2(dy, dx) - theta1;
    theta2 = std::remainder(theta2, 2.0 * std::acos(-1.0));

    double cos2 = std::cos(theta2);
    if (cos2 >= 1.0 - 1e-9) return IKStatus::SingularStraight;
    if (cos2 <= -1.0 + 1e-9) return IKStatus::SingularFolded;

    result.q1 = theta1;
    result.q2 = theta2;

    return IKStatus::Success;
}
