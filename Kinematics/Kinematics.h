#pragma once
#include "ArmTypes.h"
#include "KinematicsExport.h"

[[nodiscard]] KINEMATICS_API FKResult ForwardSolve(ArmLengthStruct length, ArmAngleStruct angle);
// Result data is valid only on Success. Valid scaffold calls return NotImplemented.
[[nodiscard]] KINEMATICS_API IKResult InverseSolve(ArmLengthStruct length, Point2D target, bool elbowUp);
[[nodiscard]] KINEMATICS_API IKResult InverseSolve(ArmLengthStruct length, ArmPosStruct pos);
