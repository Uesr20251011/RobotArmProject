#include "pch.h"
#include "RobotArm.h"
#include <cmath>

RobotArm::RobotArm() : armL{100.0, 100.0}, armQ{0.0, 0.0}, armP{} { UpdatePosition(); }
RobotArm::~RobotArm() = default;

void RobotArm::SetPara(ArmLengthStruct length, ArmAngleStruct angle, ArmPosStruct pos)
{
    armL = length;
    armQ = angle;
    armP = pos;
}
void RobotArm::SetPara(ArmLengthStruct length, ArmAngleStruct angle)
{
    armL = length;
    SetPara(angle);
}
void RobotArm::SetPara(ArmAngleStruct angle)
{
    armQ = angle;
    UpdatePosition();
}
ArmLengthStruct RobotArm::QueryLength() const { return armL; }
ArmAngleStruct RobotArm::QueryAngle() const { return armQ; }
Point2D RobotArm::QueryPosL1() const { return armP.L1; }
Point2D RobotArm::QueryPosL2() const { return armP.L2; }
ArmPosStruct RobotArm::QueryPos() const { return armP; }
void RobotArm::UpdatePosition()
{
    armP.L1 = {armL.length1 * std::cos(armQ.q1), armL.length1 * std::sin(armQ.q1)};
    const double endAngle = armQ.q1 + armQ.q2;
    armP.L2 = {armP.L1.x + armL.length2 * std::cos(endAngle),
               armP.L1.y + armL.length2 * std::sin(endAngle)};
}
