#pragma once
#include "ArmTypes.h"
#include "KinematicsExport.h"

KINEMATICS_API ArmPosStruct ForwardSolve(ArmLengthStruct length, ArmAngleStruct angle);
// Failure preserves result. Inverse solvers are unimplemented in the scaffold.
KINEMATICS_API bool InverseSolve(ArmLengthStruct length, Point2D target,
                                 bool elbowUp, ArmAngleStruct& result);
KINEMATICS_API bool InverseSolve(ArmLengthStruct length, ArmPosStruct pos,
                                 ArmAngleStruct& result);
