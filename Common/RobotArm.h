#pragma once
#include "ArmTypes.h"
#include "CommonExport.h"

class COMMON_API RobotArm {
public:
    RobotArm();
    ~RobotArm();
    // Failed setters leave every member unchanged.
    [[nodiscard]] Status SetPara(ArmLengthStruct length, ArmAngleStruct angle, ArmPosStruct pos);
    [[nodiscard]] Status SetPara(ArmLengthStruct length, ArmAngleStruct angle);
    [[nodiscard]] Status SetPara(ArmAngleStruct angle);
    ArmLengthStruct QueryLength() const;
    ArmAngleStruct QueryAngle() const;
    Point2D QueryPosL1() const;
    Point2D QueryPosL2() const;
    ArmPosStruct QueryPos() const;
private:
    ArmLengthStruct armL;
    ArmAngleStruct armQ;
    ArmPosStruct armP;
};
