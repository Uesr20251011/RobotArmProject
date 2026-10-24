#include "pch.h"
#include "Kinematics.h"
#include "RobotArm.h"

ArmPosStruct ForwardSolve(ArmLengthStruct length, ArmAngleStruct angle)
{
    RobotArm arm;
    arm.SetPara(length, angle);
    return arm.QueryPos();
}
bool InverseSolve(ArmLengthStruct /*length*/, Point2D /*target*/,
                  bool /*elbowUp*/, ArmAngleStruct& /*result*/)
{
    // Implementation belongs to the kinematics workstream.
    return false;
}
bool InverseSolve(ArmLengthStruct /*length*/, ArmPosStruct /*pos*/, ArmAngleStruct& /*result*/)
{
    // Implementation belongs to the kinematics workstream.
    return false;
}
