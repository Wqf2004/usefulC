# 在vscode中运行C程序

## 1. 安装插件

首先，你需要在VSCode中安装一些插件来支持C语言开发：

1. **C/C++**：由Microsoft提供的官方插件，用于C/C++语言的支持。
2. **Code Runner**：用于快速运行代码的插件。

安装步骤：

1. 打开VSCode。
2. 点击左侧活动栏中的扩展图标（四个方块组成的图标）。
3. 在搜索框中输入插件名称并安装。

## 2. 构建与运行

打开一个.c文件，先使用快捷键 Ctrl+Shift+B，它会调用.vscode/task.json中的指令完成构建，然后右键选择 Run Code（Code Runner 扩展）运行。
