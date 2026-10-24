#pragma once
#include "ArmTypes.h"
#include "CommonExport.h"

class COMMON_API RobotArm {
public:
    RobotArm();
    ~RobotArm();
    void SetPara(ArmLengthStruct length, ArmAngleStruct angle, ArmPosStruct pos);
    void SetPara(ArmLengthStruct length, ArmAngleStruct angle);
    void SetPara(ArmAngleStruct angle);
    ArmLengthStruct QueryLength() const;
    ArmAngleStruct QueryAngle() const;
    Point2D QueryPosL1() const;
    Point2D QueryPosL2() const;
    ArmPosStruct QueryPos() const;
private:
    void UpdatePosition();
    ArmLengthStruct armL;
    ArmAngleStruct armQ;
    ArmPosStruct armP;
};
