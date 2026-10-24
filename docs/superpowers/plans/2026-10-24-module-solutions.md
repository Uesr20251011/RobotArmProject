# 独立模块解决方案调整

目标：Common、Kinematics、Trajectory、RobotArmUI 都提供独立、受 Git 管理的 `.sln`，复用现有工程和公共 props。

## 方案与边界

| 解决方案 | 包含项目 |
| --- | --- |
| Common/Common.sln | Common |
| Kinematics/Kinematics.sln | Kinematics、Common |
| Trajectory/Trajectory.sln | Trajectory、Common、Kinematics |
| RobotArmUI/RobotArmUI.sln | RobotArmUI、Common、Kinematics、Trajectory |

每个入口以目标模块为首个项目，并提供 Debug / Release x64 配置。解决方案使用已有 vcxproj 的 GUID、相对路径和 ProjectReference；不复制源码或项目。输出继续由 commonlibdll 的 props 管理。

库解决方案直接纳入 Git，纯库开发不依赖 Qt 或 qmake。界面解决方案纳入 Git，界面 vcxproj 仍由 `.pro` 生成并忽略本机路径；原生成脚本继续维护根解决方案和界面解决方案，重复执行不得改变受管理的界面 sln。

## 实施与验收

1. 在最新 main 上创建 feature/standalone-library-solutions。
2. 增加三个库的 sln，取消界面 sln 的忽略规则并提交。
3. 增加独立库解决方案验证入口，并接入现有 Build.ps1 -Test，以便 CI 覆盖每个库的独立构建。
4. 更新 README、架构和协作指南，列出各角色应打开的 sln。
5. 验证三个库分别在 Debug / Release x64 下重新生成；验证独立 UI sln 和根解决方案；检查重复生成的稳定性。
6. 独立审查、提交、推送、创建 PR、等待自动检查，通过后 squash 合并并同步本地 main。
