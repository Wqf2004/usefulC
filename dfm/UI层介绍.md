# DFM 命令行 UI 层

## 职责

UI 层负责文本菜单、输入提示、输入范围检查及结果展示；学生、成绩和账号规则交给业务层 `utility_*` 接口处理。程序入口 `src/main.c` 只调用 `ui_run()`，菜单实现位于 `src/ui.c`，公开入口声明在 `include/ui.h`。

## 使用流程

启动后可选择登录、注册或退出。登录成功后提供以下操作：

1. 按学号或姓名查询学生
2. 按宿舍号查询学生
3. 查询学生全部成绩及实得学分总和
4. 录入成绩并由业务层计算综合成绩与实得学分
5. 删除学生及其成绩，并记录删除数据
6. 按综合成绩或实得学分排序
7. 统计各课程平均综合成绩

数字选项必须处于菜单给出的范围内；必填字段不能为空，单字段不能包含空白字符。实验成绩输入 `-1` 表示课程无实验，其余成绩需在 `0` 到 `100` 之间。

## 数据文件

UI 会根据当前工作目录查找学生数据文件：从仓库根目录启动时使用 `data/`；从 `dfm/` 目录启动时使用 `../data/`。账号、登录日志和删除日志写入同一数据目录下的 `users.txt`、`login.log` 和 `deleted_students.log`。密码哈希和认证细节由业务层处理。

学生记录按项目需求手动维护在 `a.txt` 中，不通过 UI 新增。录入成绩时，学号必须已存在于该文件；如果不存在，UI 会提示先补充学生记录，避免把业务层的 `UTILITY_NOT_FOUND` 误认为文件错误。

## 构建与运行

在仓库根目录的 PowerShell 终端构建：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\dfm\build.ps1
```

构建完成后可从仓库根目录运行：

```powershell
.\dfm\build\main.exe
```

也可先切换到 `dfm/` 再运行 `build\main.exe`；UI 会按当前目录选择对应的数据路径。

## UI 集成测试

在仓库根目录编译并运行：

```powershell
gcc dfm/src/ui.c dfm/src/utility.c dfm/src/utility_query.c dfm/src/utility_delete.c dfm/src/grade.c dfm/src/file_op.c dfm/src/security.c dfm/tests/ui_test.c -Idfm/include -finput-charset=GBK -fexec-charset=GBK -std=c17 -Wall -Wextra -Wpedantic -lbcrypt -o dfm/build/ui_test.exe
if ($LASTEXITCODE -eq 0) { .\dfm\build\ui_test.exe }
```

测试会在 `dfm/build/` 下创建按进程号区分的临时工作目录，使用模拟学生和成绩文件端到端执行注册、登录、成绩查询及成绩录入。边界场景包括菜单非法选项、空字段、带空格字段、过短密码、重复注册、错误密码登录、学分/成绩越界后重输、无实验成绩 `-1`，以及 `0` 和 `100` 分；同时验证未知学号提示、写入记录、查询结果、密码非明文保存和登录日志。成功结束后会清理该测试目录，不触碰仓库的 `data/` 业务文件。
