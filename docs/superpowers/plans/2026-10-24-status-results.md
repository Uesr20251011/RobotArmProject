# 统一状态与计算结果接口

依据：Issue #2 的用户评论 https://github.com/Uesr20251011/RobotArmProject/issues/2#issuecomment-6379340143 。本次只迁移接口与调用，不实现尚未完成的逆解和轨迹算法。

## 接口设计

- Common/Status.h 定义固定 int 底层类型的 Status：Success、InvalidParameter、InvalidInput、NotInitialized、Unreachable、NonUniqueSolution、NumericalError，另增加 NotImplemented 表示现有算法骨架。
- 三个 RobotArm::SetPara 重载返回 Status；先验证并计算候选状态，全部成功后才提交，失败不修改任何成员。默认机械臂仍为已经初始化的 100/100、0/0 状态。
- ForwardSolve 返回 FKResult（status、pos）；两个 InverseSolve 返回 IKResult（status、angle），删除输出引用参数。
- PlanJoint、PlanCartesian 返回 TrajectoryResult（status、trajectory），删除输出引用参数。
- 三类计算结果默认 status 为 NotInitialized，数据全部值初始化；只有 Success 的数据有效，失败轨迹为空。
- 只读取已初始化对象的 Query 方法保持现有返回类型；不为纯查询重复包装状态。
- 长度必须为正且有限；非有限输入返回 InvalidInput，非正长度/时间返回 InvalidParameter；有限输入导致角度求和或位置计算溢出返回 NumericalError。全参数 SetPara 保留显式保存位置的语义，仅检查坐标有限，几何一致性与容差仍由 Issue #2 后续约定。
- 逆解/规划先检查输入，再返回 NotImplemented；不把未完成误报为 Unreachable 或 NumericalError。构型、容差阈值、插值和唯一解规则不在此次迁移中擅自确定。

## 实施与验证

1. 在 feature/unified-status-results 分支先迁移测试，验证旧 API 无法满足新的状态与结果契约。
2. 定义公共状态及结果结构体，更新 Common 公共头文件发布清单。
3. 实现事务式 SetPara，迁移正解、逆解骨架、规划骨架及界面正解调用。
4. 验证默认结果初始化、非法输入分类、失败状态不变、溢出、有效正解及空的失败轨迹。
5. 更新架构、协作说明和示例；原始设计文档保留为历史参考，当前接口以架构文档和头文件为准。
6. 运行四个独立解决方案及根解决方案的 Debug / Release 构建、DLL 契约和 Qt 启动检查，独立审查后提交 PR。
7. 保持 Issue #2 打开；枚举与返回值迁移不等于其余数值约定、算法和三人确认已完成。
