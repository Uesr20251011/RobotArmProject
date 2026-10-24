# 公共库发布目录

三个 DLL 编译后由共享 props 自动发布到这里。源码头文件在 Common、Kinematics、Trajectory 中编辑。

三个模块分别位于 common/、kinematics/、trajectory/，都采用以下布局：

```text
<module>/
  include/           # 公共头文件副本
  bin/x64/Debug/     # *d.dll / PDB
  bin/x64/Release/   # *.dll / PDB
  lib/x64/Debug/     # *d.lib
  lib/x64/Release/   # *.lib
```

发布物已被 .gitignore 排除，克隆后先构建生成。
