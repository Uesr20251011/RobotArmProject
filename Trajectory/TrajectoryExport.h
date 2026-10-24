#pragma once
#ifdef TRAJECTORY_EXPORTS
#define TRAJECTORY_API __declspec(dllexport)
#else
#define TRAJECTORY_API __declspec(dllimport)
#endif
