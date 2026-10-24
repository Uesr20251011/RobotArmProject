#pragma once
#ifdef KINEMATICS_EXPORTS
#define KINEMATICS_API __declspec(dllexport)
#else
#define KINEMATICS_API __declspec(dllimport)
#endif
