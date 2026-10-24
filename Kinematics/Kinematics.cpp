#include "pch.h"
#include "Kinematics.h"
#include "RobotArm.h"
#include <cmath>

FKResult ForwardSolve(ArmLengthStruct length, ArmAngleStruct angle)
{
    RobotArm arm;
    const auto status = arm.SetPara(length, angle);
    if (status != Status::Success) return {status};
    return {Status::Success, arm.QueryPos()};
}
IKResult InverseSolve(ArmLengthStruct length, Point2D target, bool /*elbowUp*/)
{
    if (!std::isfinite(length.length1) || !std::isfinite(length.length2)
        || !std::isfinite(target.x) || !std::isfinite(target.y)) return {Status::InvalidInput};
    if (length.length1 <= 0.0 || length.length2 <= 0.0) return {Status::InvalidParameter};
    // Implementation belongs to the kinematics workstream.
    return {Status::NotImplemented};
}
IKResult InverseSolve(ArmLengthStruct length, ArmPosStruct pos)
{
    if (!std::isfinite(length.length1) || !std::isfinite(length.length2)
        || !std::isfinite(pos.L1.x) || !std::isfinite(pos.L1.y)
        || !std::isfinite(pos.L2.x) || !std::isfinite(pos.L2.y)) return {Status::InvalidInput};
    if (length.length1 <= 0.0 || length.length2 <= 0.0) return {Status::InvalidParameter};
    // Implementation belongs to the kinematics workstream.
    return {Status::NotImplemented};
}
