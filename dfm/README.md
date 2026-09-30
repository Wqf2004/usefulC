# 学生信息管理系统

基于 C17 的学生信息与成绩管理命令行程序。系统使用文本文件保存学生、成绩和账号数据，菜单层负责交互，业务层负责校验与数据处理。

## 功能

- 注册账号、登录验证和登录日志记录；密码以带随机盐的 PBKDF2-SHA256 摘要保存。
- 按学号或姓名查询学生，或按宿舍号查询学生列表。
- 查询学生的课程成绩及实得学分总和。
- 录入成绩并自动计算综合成绩和实得学分。
- 删除学生时同步删除成绩，并把删除记录写入日志。
- 按综合成绩或实得学分排序，统计各课程平均综合成绩。
- 校验学生记录中的手机号和成绩文件字段。

学生基本信息按项目要求手动维护在 `data/a.txt`；程序菜单不新增学生。录入成绩时，学号必须已存在于学生文件。

## 环境

- Windows
- GCC（支持 C17）
- Windows BCrypt 库（链接参数 `-lbcrypt`）
- PowerShell

源码使用 GBK 编码，构建命令通过 `-finput-charset=GBK -fexec-charset=GBK` 指定字符集。

## 构建与运行

在仓库根目录运行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\dfm\build.ps1
.\dfm\build\main.exe
```

也可以在 `dfm/` 目录中运行 `build.ps1` 并启动 `build\main.exe`。UI 根据当前工作目录选择学生数据路径：从仓库根目录使用 `data/`，从 `dfm/` 目录使用 `../data/`。

程序启动后可注册或登录。登录后按菜单编号使用查询、成绩录入、删除、排序和统计功能。成绩录入要求课程编号、课程名称等字段不包含空白字符；无实验课程的实验成绩输入 `-1`。

## 数据格式

文件采用空白分隔格式，可包含首行表头。每个字段不能包含空格或制表符。

`data/a.txt` 的字段顺序：

```text
学号 姓名 性别 宿舍号码 电话号码
1001 Alice F 101 13800138000
```

`data/b.txt` 的字段顺序：

```text
学号 课程编号 课程名称 学分 平时成绩 实验成绩 卷面成绩 综合成绩 实得学分
1001 MTH Math 3 80 -1 90 87 2.4
```

手机号要求为 11 位数字，且以 13 至 19 开头。实验成绩为 `-1` 表示课程没有实验；有实验时，综合成绩为平时成绩的 15%、实验成绩的 15% 与卷面成绩的 70% 之和。无实验时平时成绩占 30%，卷面成绩占 70%。

实得学分按综合成绩折算：90 分及以上为课程学分的 100%，80 至 90 分以下为 80%，70 至 80 分以下为 75%，60 至 70 分以下为 60%，低于 60 分为 0。

程序首次注册后会在数据目录生成 `users.txt`；登录日志写入 `login.log`，删除记录写入 `deleted_students.log`。

## 测试

在仓库根目录使用 GCC 编译并运行全部测试：

```powershell
gcc dfm/src/utility.c dfm/src/utility_query.c dfm/src/utility_delete.c dfm/src/grade.c dfm/src/file_op.c dfm/src/security.c dfm/tests/utility_test.c -Idfm/include -finput-charset=GBK -fexec-charset=GBK -std=c17 -Wall -Wextra -Wpedantic -lbcrypt -o dfm/build/utility_test.exe
if ($LASTEXITCODE -eq 0) { .\dfm\build\utility_test.exe }

gcc dfm/src/file_op.c dfm/tests/file_op_delete_test.c -Idfm/include -finput-charset=GBK -fexec-charset=GBK -std=c17 -Wall -Wextra -Wpedantic -o dfm/build/file_op_delete_test.exe
if ($LASTEXITCODE -eq 0) { .\dfm\build\file_op_delete_test.exe }

gcc dfm/src/ui.c dfm/src/utility.c dfm/src/utility_query.c dfm/src/utility_delete.c dfm/src/grade.c dfm/src/file_op.c dfm/src/security.c dfm/tests/ui_test.c -Idfm/include -finput-charset=GBK -fexec-charset=GBK -std=c17 -Wall -Wextra -Wpedantic -lbcrypt -o dfm/build/ui_test.exe
if ($LASTEXITCODE -eq 0) { .\dfm\build\ui_test.exe }
```

业务测试覆盖数据校验、查询、成绩计算、排序、统计、录入、删除和认证。UI 集成测试在隔离的临时目录中验证菜单输入边界、注册登录、成绩录入和查询，不修改 `data/` 中的业务文件。

## 代码结构

- `src/main.c`：程序入口。
- `src/ui.c`、`include/ui.h`：命令行菜单、输入和结果展示。
- `src/utility.c`、`src/utility_query.c`、`src/utility_delete.c`：业务层实现。
- `src/grade.c`：成绩等级及计算规则。
- `src/file_op.c`：文本文件行操作。
- `tests/`：业务、文件操作及 UI 集成测试。

更多说明见 [业务层介绍](业务层介绍.md) 和 [UI 层介绍](UI层介绍.md)。
