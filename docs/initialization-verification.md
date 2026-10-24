# 初始化验证记录

日期：2026-10-24。

环境：Windows x64，Visual Studio 2022 Community 17.13 / MSVC v143 14.43，Qt 6.11.2 MSVC 2022 x64。

## 本机验证

| 检查 | 结果 |
| --- | --- |
| Debug x64 整体构建 | 通过 |
| Release x64 整体构建 | 通过 |
| 两种配置下的 DLL 调用与状态契约检查 | 通过 |
| 两种配置下 Qt 窗口构造、显示、事件循环及正常退出 | 通过 |
| 发布 DLL 的导出表 | Common 12 项、Kinematics 3 项、Trajectory 5 项 |
| 只从 Git 暂存源码导出到全新、含空格的路径，重新生成并构建 Debug | 通过，DLL 调用和 Qt 启动检查通过 |
| 提交差异格式检查 | 通过 |
| 独立只读工程审查 | 未发现重要问题 |
| GitHub Actions Windows x64 Debug / Release | 两个任务均通过 |

全新路径验证位于系统临时目录，MSBuild 提示临时目录可能影响增量构建；正常项目目录的 Debug / Release 构建没有该提示。

## 验证入口

```powershell
.\scripts\Build.ps1 -QtRoot $env:QT_ROOT -Configuration Debug -Test
.\scripts\Build.ps1 -QtRoot $env:QT_ROOT -Configuration Release -Test
```

启动检查使用随程序部署的 Windows 平台插件。初始化时曾尝试 offscreen，但该插件不在默认部署集中，已改为 Windows 插件并验证两种配置通过。

## 范围

此记录证明工程骨架、导出导入与运行库部署链路可用。逆运动学和轨迹规划保留明确失败的骨架入口，界面交互、绘制和动画仍由对应角色开发。相关算法完成时需要更新初始化断言并增加算法测试。

GitHub 公有仓库已创建并上传。[远端检查第 40053348829 次运行](https://github.com/Uesr20251011/RobotArmProject/actions/runs/40053348829) 对工程版本 `9c2b6283c9ee8786656c0dab579622019187d40b` 的 Debug / Release 两个任务均通过，包括库编译、Qt 工程生成、DLL 调用检查和 Windows 平台插件启动检查。随后提交仅补充文档记录。

远端安装工具固定到 aqtinstall 提交 `9e49c82edc6d946db376dec907cca5b4b486eec5`，以支持 Qt 6.11 的 Windows 下载目录。相关背景见 [aqtinstall 官方变更记录](https://aqtinstall.readthedocs.io/en/latest/CHANGELOG.html)。后续提交的实时状态见 [GitHub Actions](https://github.com/Uesr20251011/RobotArmProject/actions)。
