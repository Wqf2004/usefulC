# usefulC

通过一个简单的项目【含文件操作】一步一步（step by step）to help you learn how to understand the knowledge tree of the C language.

该项目主要在 vscode 下运行，请先阅读[在vscode中运行C程序](./在vscode中运行C程序.md)

## 使用vscode参与本项目

电脑需要先安装git
1. Fork: 在 https://github.com/Wqf2004/usefulC 页面下Fork，到自己的Github账号上
2. Clone 自己的 fork，先选择目录，然后在git中使用`git clone xxx.git`下载到本地
3. 关联 upstream: vscode打开项目，在顶部输入框敲 `>add remote`，粘贴上面的链接，回车
4. 建功能分支：点左下角状态栏分支名（默认显示main），点击“创建新分支”，输入分支名称，回车
5. 写/改代码 → 暂存 → 提交（输入的消息内容参考 Commit 规范）
6. 推送
7. 发PR，到上面的链接处

### Commit 规范

格式：`<type>(<scope>): <description>`

- `type`：提交类型
- `scope`：可选，表示影响范围，如 `build`、`docs`、`file`
- `description`：简短说明，用祈使句，首字母小写，不加句号

| type    | 用途                    | 举例                                              |
| ------- | ----------------------- | ------------------------------------------------- |
| `feat`  | 新功能                  | `feat(file): add file read/write example`         |
| `fix`   | bug 修复                | `fix(build): create build directory before gcc`   |
| `build` | 构建系统（Makefile 等） | `build(vscode): update GCC flags in tasks.json`   |
| `chore` | 杂项（初始化、配置）    | `chore: initialize project structure`             |
| `docs`  | 文档                    | `docs: add commit convention`                     |
| `test`  | 测试                    | `test(file): add tests for file open failure`     |
