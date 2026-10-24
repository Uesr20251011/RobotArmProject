#pragma once
#include "Status.h"

struct ArmLengthStruct { double length1{}; double length2{}; };
// Radians. q2 is relative to the first link.
struct ArmAngleStruct { double q1{}; double q2{}; };
struct Point2D { double x{}; double y{}; };
struct ArmPosStruct { Point2D L1{}; Point2D L2{}; };

struct FKResult {
    Status status{Status::NotInitialized};
    ArmPosStruct pos{};
};

struct IKResult {
    Status status{Status::NotInitialized};
    ArmAngleStruct angle{};
};
