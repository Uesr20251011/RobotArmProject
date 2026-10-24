# 开发与分工约定

首次使用 GitHub 请先阅读 [新手协作指南](docs/GitHub多人协作指南.md)。三位成员采用同一仓库、短任务分支、PR 合并的流程。

## 分工边界

| 角色 | 主要修改范围 | 首批任务 | 审查搭档 |
| --- | --- | --- | --- |
| 界面 | RobotArmUI/ | 参数输入、绘制、轨迹展示、播放暂停 | 运动学或轨迹负责人 |
| 运动学 | Kinematics/、Common/ | 逆解两种重载、构型与不可达处理；维护状态契约 | 轨迹负责人 |
| 轨迹规划 | Trajectory/ | 关节空间及直线规划、时间采样、失败清理 | 运动学负责人 |

Common、公共 props、根解决方案、构建脚本属于共同基础；修改前先在 Issue 中说明影响。公共头文件签名变更需要其他两个角色确认，并同步改调用点和测试。

界面负责人打开 RobotArmUI/RobotArmUI.sln（首次先生成 Qt 工程）；运动学负责人打开 Kinematics/Kinematics.sln，需要时单独打开 Common/Common.sln；轨迹负责人打开 Trajectory/Trajectory.sln。各库入口包含自己的依赖项目，纯库开发不要求安装 Qt。整体联调继续使用根 RobotArmProject.sln。

## 分支与提交

- main：可构建的集成版本，团队约定通过 PR 更新。
- feature/ui-任务名、feature/kinematics-任务名、feature/trajectory-任务名：单个功能的工作分支。
- fix/问题名：修复分支。
- 一个分支解决一个问题，合并后新任务从最新 main 创建新分支。
- 提交说明写清结果，如 `feat(kinematics): add endpoint inverse solver`。
- PR 中说明改动、验证结果、是否影响公共接口；至少请另一位成员审查。

## 接口与质量

1. 保留原设计的方法签名，类型语义见 [架构约定](docs/architecture.md)。
2. 逆解失败保持输出角度不变；规划失败清空结果；规划不修改输入机械臂。
3. 采样策略需要验证起点、终点、非整数倍时长和无效时间参数。
4. 新功能需补充有意义的验证。初始化测试中标注 scaffold 的断言要随对应算法完成改成算法测试，不能为保持旧测试通过而保留无条件失败。
5. 合并前运行 Debug 和 Release 的 `Build.ps1 -Test`，查看 GitHub Actions 状态。
6. 构建目录、库二进制、发布头文件、VS 用户设置不纳入版本管理。

## 测试归属

当前 ArchitectureSmoke 检查公共对象、DLL 导出及调用、骨架失败契约；随算法开发在 tests 中扩展断言，或增加独立测试工程。新增工程由维护构建脚本的成员同步更新 Generate-VS.ps1 的工程清单，并同步受影响的独立 sln。Test-LibrarySolutions.ps1 可在没有 Qt 的环境下验证三个库的独立重新生成；Build.ps1 -Test 及 CI 会覆盖独立库、独立界面和整体联调入口。

## 仓库权限

公有仓库可被任何人查看。直接推送项目分支需要仓库所有者邀请为 Collaborator；尚未被邀请的成员可以先克隆阅读。当前文档只按角色分工，未绑定具体 GitHub 账号。main 的 PR 审核约定是团队流程，是否强制执行由仓库所有者在分支规则中设置。
