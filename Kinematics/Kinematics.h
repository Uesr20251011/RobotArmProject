#pragma once
#include "ArmTypes.h"
#include "KinematicsExport.h"

enum class IKStatus
{
    Success,
    InvalidLength,
    Unreachable,
    SingularStraight,
    SingularFolded
};

KINEMATICS_API ArmPosStruct ForwardSolve(ArmLengthStruct length, ArmAngleStruct angle);
// Only Success updates result. elbowUp selects negative q2.
KINEMATICS_API IKStatus InverseSolve(ArmLengthStruct length, Point2D target,
                                    bool elbowUp, ArmAngleStruct& result);
KINEMATICS_API IKStatus InverseSolve(ArmLengthStruct length, ArmPosStruct pos,
                                    ArmAngleStruct& result);
