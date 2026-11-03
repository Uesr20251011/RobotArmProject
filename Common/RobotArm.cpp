#include "pch.h"
#include "RobotArm.h"
#include "Kinematics.h"

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
    armP = ForwardSolve(armL, armQ);
}
