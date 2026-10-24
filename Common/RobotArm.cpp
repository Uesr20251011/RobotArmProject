#include "pch.h"
#include "RobotArm.h"
#include <cmath>

namespace {
Status ValidateParameters(ArmLengthStruct length, ArmAngleStruct angle)
{
    if (!std::isfinite(length.length1) || !std::isfinite(length.length2)
        || !std::isfinite(angle.q1) || !std::isfinite(angle.q2)) {
        return Status::InvalidInput;
    }
    if (length.length1 <= 0.0 || length.length2 <= 0.0) return Status::InvalidParameter;
    return Status::Success;
}
bool IsFinite(ArmPosStruct pos)
{
    return std::isfinite(pos.L1.x) && std::isfinite(pos.L1.y)
        && std::isfinite(pos.L2.x) && std::isfinite(pos.L2.y);
}
}

RobotArm::RobotArm() : armL{100.0, 100.0}, armQ{0.0, 0.0}, armP{}
{
    // These fixed finite defaults always produce a valid initial state.
    (void)SetPara(armL, armQ);
}
RobotArm::~RobotArm() = default;

Status RobotArm::SetPara(ArmLengthStruct length, ArmAngleStruct angle, ArmPosStruct pos)
{
    if (!IsFinite(pos)) return Status::InvalidInput;
    const auto status = ValidateParameters(length, angle);
    if (status != Status::Success) return status;
    // Preserve the explicit-state overload's supplied-position semantics.
    armL = length;
    armQ = angle;
    armP = pos;
    return Status::Success;
}
Status RobotArm::SetPara(ArmLengthStruct length, ArmAngleStruct angle)
{
    const auto status = ValidateParameters(length, angle);
    if (status != Status::Success) return status;
    const double endAngle = angle.q1 + angle.q2;
    if (!std::isfinite(endAngle)) return Status::NumericalError;
    ArmPosStruct pos{};
    pos.L1 = {length.length1 * std::cos(angle.q1), length.length1 * std::sin(angle.q1)};
    pos.L2 = {pos.L1.x + length.length2 * std::cos(endAngle),
              pos.L1.y + length.length2 * std::sin(endAngle)};
    if (!IsFinite(pos)) return Status::NumericalError;
    // Commit only after the entire candidate state has been validated.
    armL = length;
    armQ = angle;
    armP = pos;
    return Status::Success;
}
Status RobotArm::SetPara(ArmAngleStruct angle)
{
    return SetPara(armL, angle);
}
ArmLengthStruct RobotArm::QueryLength() const { return armL; }
ArmAngleStruct RobotArm::QueryAngle() const { return armQ; }
Point2D RobotArm::QueryPosL1() const { return armP.L1; }
Point2D RobotArm::QueryPosL2() const { return armP.L2; }
ArmPosStruct RobotArm::QueryPos() const { return armP; }
