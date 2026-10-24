# RobotArmProject

二连杆机械臂课程项目，采用 **Visual Studio 原生 DLL + Qt Widgets**，按三人分工初始化。

| 模块 | 工程 | 负责内容 |
| --- | --- | --- |
| 公共状态 | Common/Common.vcxproj | 公共结构、RobotArm 状态，运动学负责人牵头维护 |
| 运动学 | Kinematics/Kinematics.vcxproj | 正解、两种逆解 |
| 轨迹规划 | Trajectory/Trajectory.vcxproj | 关节空间及笛卡尔直线轨迹 |
| 界面 | RobotArmUI/RobotArmUI.pro | 参数操作、机械臂绘制、轨迹及动画 |

三个库由 VS 2022 的 **Windows DLL with Exports（带导出项的 DLL）** 模板创建，具有独立导出宏和原生项目引用。界面项目由 Qt `.pro` 经 qmake 转换为 VS 工程及解决方案。

## 新手先读

- [GitHub 多人协作指南](docs/GitHub多人协作指南.md)：从加入仓库到第一次 PR，含图形界面操作和命令速查。
- [开发与分工约定](CONTRIBUTING.md)
- [架构和接口约定](docs/architecture.md)
- [原始设计](面向对象作业设计.md)

## 构建与运行

安装 VS 2022 的“使用 C++ 的桌面开发”（v143、Windows SDK）以及 **Qt 6.11.2 / MSVC 2022 64-bit**。使用 PowerShell 7 或 Windows PowerShell 5.1，在项目根目录执行：

```powershell
# 修改成自己电脑上的 Qt MSVC kit 路径，不能选择 mingw_64。
$env:QT_ROOT = 'E:\Tools_E\QtOrginal\6.11.2\msvc2022_64'
.\scripts\Build.ps1 -Configuration Debug -Test
.\build\bin\x64\Debug\RobotArmUI.exe
```

如果系统阻止运行本项目脚本，可只为当前 PowerShell 窗口执行 `Set-ExecutionPolicy -Scope Process Bypass`，然后重试。

只生成 VS 工程：

```powershell
.\scripts\Generate-VS.ps1 -QtRoot $env:QT_ROOT
```

生成后打开根目录 **RobotArmProject.sln**，选择 Debug / x64，以 RobotArmUI 为启动项目。也可以打开 **RobotArmUI/RobotArmUI.sln**。脚本生成的 Qt 工程直接使用 qmake 的 MOC/UIC 构建规则，不要求安装 Qt VS Tools 扩展。新增源文件或修改 `.pro` 后，关闭 VS 解决方案，重新运行生成脚本再打开。

Release 验证：

```powershell
.\scripts\Build.ps1 -Configuration Release -Test
```

`-Test` 会运行跨 DLL 契约检查，以及使用已部署 Windows 平台插件的 Qt 启动检查。

## 公共库目录

```text
Common/ Kinematics/ Trajectory/    原生 VS DLL 源码与工程
RobotArmUI/                       .pro、Designer .ui、窗口源码
commonlibdll/
  BuildSettings.props             编译选项、CRT、路径根目录
  LibraryOutput.props             DLL、LIB、头文件发布
  RobotArmLib.props                库引用、搜索路径和运行 DLL 复制
  common/                         Common 发布物
  kinematics/                     Kinematics 发布物
  trajectory/                     Trajectory 发布物
docs/                             架构、协作与计划文档
scripts/                          生成和构建入口
tests/                            跨 DLL 冒烟检查
build/                            本机构建产物，Git 忽略
```

每个库输出到 `commonlibdll/<模块>/bin/x64/<配置>` 和 `lib/x64/<配置>`，公共头文件发布到对应 `include`。Debug 库带 `d` 后缀，Release 不带。界面输出到 `build/bin/x64/<配置>`，构建时自动复制项目 DLL 并部署 Qt 运行库。

请编辑模块源码目录里的头文件，commonlibdll 下的头文件副本和二进制无需手工复制，也不提交 Git。

## 当前进度

已实现公共状态及正解基础调用，可编译并启动界面。**逆解、轨迹规划、完整操作界面和绘制动画仍需按分工开发**。未实现的逆解返回 false，轨迹规划返回 false 并清空结果，避免把空骨架当成可用算法。

提交与 PR 会触发 Windows Debug / Release 构建及启动检查。检查范围以工作流实际结果为准。
