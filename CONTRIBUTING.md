# 为 Tianole OS 贡献

首先，非常感谢你愿意花时间来为这个项目做出贡献！我们欢迎任何形式的帮助。

本项目的核心目标是学习和交流，因此我们对新手非常友好。如果你在贡献过程中遇到任何问题，请随时通过 [GitHub Issues](https://github.com/microindole/tianole/issues) 与我们联系。

## 💬 行为准则

在参与社区贡献之前，请花一点时间阅读我们的 [**行为准则 (Code of Conduct)**](./CODE_OF_CONDUCT.md)。我们希望为所有人创造一个友好、互相尊重、具有建设性的社区环境。

## 如何开始？

你可以从 [README.md](./README.md) 中的“主要贡献方向”部分寻找你感兴趣的任务。我们也会在 [GitHub Issues](https://github.com/microindole/tianole/issues) 中创建一些带有 `help-wanted` 或 `good-first-issue` 标签的任务，这些任务非常适合作为你的第一个贡献。

### 报告 Bug

如果你在运行或编译 Tianole OS 时发现了问题，请通过 [GitHub Issues](https://github.com/microindole/tianole/issues) 提交一份 Bug 报告。一份好的 Bug 报告应包含：

- **清晰的标题**：简要概括你遇到的问题。
- **复现步骤**：详细描述你是如何触发这个 Bug 的。
- **期望的结果**：你认为应该发生什么。
- **实际的结果**：实际发生了什么，最好能附上截图或日志。
- **你的环境**：你使用的操作系统、QEMU 版本、GCC 版本等。

### 提交功能建议

如果你对项目有任何新的想法或建议，也欢迎通过 [GitHub Issues](https://github.com/microindole/tianole/issues) 告诉我们。请详细描述你的想法以及它能解决什么问题。

## 提交你的贡献 (Pull Request)

我们通过 GitHub 的 Pull Request (PR) 流程来接受代码贡献。

1. **Fork 仓库**：点击仓库右上角的 "Fork" 按钮，将主仓库复制一份到你自己的 GitHub 账号下。

2. **克隆你的 Fork**：将你的 Fork 克隆到本地。

   ```bash
   git clone [https://github.com/你的用户名/tianole.git](https://github.com/你的用户名/tianole.git)
   cd tianole
   ```

3. **创建新分支**：为你的修改创建一个新的分支。一个好的分支名能清晰地描述你的工作内容，例如 `feature/exec-syscall` 或 `fix/keyboard-bug`。

   ```bash
   git checkout -b <你的分支名>
   ```

4. **进行修改**：在本地进行代码修改和开发。

5. **提交你的修改**：

   ```bash
   git add .
   git commit -m "一个清晰、有意义的提交信息"
   ```

6. **推送到你的 Fork**：

   ```bash
   git push origin <你的分支名>
   ```

7. **创建 Pull Request**：回到你的 GitHub Fork 页面，点击 "New pull request" 按钮。请在 PR 的描述中详细说明你做了哪些修改、解决了什么问题。

### 编码风格

为了保持代码库的一致性，我们希望你遵循以下简单的编码风格：

- **语言**：主要使用 C 和 NASM 汇编。
- **注释**：我们鼓励使用**中文**编写详细的注释，解释代码的意图和实现逻辑。
- **命名**：
  - 函数和变量名使用蛇形命名法 (snake_case)，例如 `init_paging`。
  - 宏和常量使用全大写，例如 `VGA_WIDTH`。
- **格式化**：
  - 使用 4 个空格进行缩进。
  - 大括号 `{` 放在函数或控制语句的同一行。

感谢你的贡献！