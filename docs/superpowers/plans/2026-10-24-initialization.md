# RobotArmProject Initialization Plan

**Goal:** 按已有设计建立可构建的四模块工程，并创建可多人协作的 GitHub 仓库。

**Architecture:** 原生 VS 导出 DLL + Qt qmake 生成的界面 VS 工程。共享 props 管理发布及消费路径，公共接口遵循原始设计。

**Tech Stack:** Windows x64 / VS 2022 v143 / C++17 / Qt 6.11.2 MSVC 2022。

**Spec:** `docs/architecture.md`，原始依据为根目录《面向对象作业设计.md》。

## 工作顺序

- [x] 用已安装 VS 的 WindowsDLLExports 模板创建 Common、Kinematics、Trajectory；保留 DLL 启动文件，替换样例导出项为设计接口。
- [x] 增加 BuildSettings.props、LibraryOutput.props、RobotArmLib.props，统一 Debug/Release 输出、头文件发布、依赖链接和运行库复制。
- [x] 增加 RobotArmUI.pro、Designer 界面与槽函数骨架；生成脚本实际调用 qmake -tp vc 并注入 props 和项目引用。
- [x] 增加跨 DLL 冒烟测试、构建脚本及 Windows CI，验证公共状态、正解和未实现入口的失败行为。
- [x] 构建 x64 Debug 和 Release，检查 DLL 导出，运行测试及 Qt 启动检查。
- [x] 编写 README、三人协作流程、PR 模板及 .gitignore，检查提交清单只包含源码与文档。
- [x] 初始化 main 分支，提交并推送当前已登录账号的公有 RobotArmProject 仓库；验证远端提交与本地一致。

## 验收命令

```powershell
.\scripts\Build.ps1 -QtRoot 'E:\Tools_E\QtOrginal\6.11.2\msvc2022_64' -Configuration Debug -Test
.\scripts\Build.ps1 -QtRoot 'E:\Tools_E\QtOrginal\6.11.2\msvc2022_64' -Configuration Release -Test
git status --short
git ls-remote origin refs/heads/main
```
