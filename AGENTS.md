# Tianole Agent Guide

这个文件是仓库内 agent 文档的入口。

## 文档位置

- 正式设计、路线图、架构说明放在 `docs/`
- 面向 agent 的任务、会话记录、检查单放在 `docs/agents/`

## 工作规则

- agent 修改代码前，先确认相关正式文档是否已经存在。
- agent 修改代码后，如果改动影响了架构边界、目录结构、构建方式、阶段计划或操作流程，必须同步更新 `docs/` 或 `docs/agents/`。
- 纯粹的小修复如果不改变这些信息，可以不额外补文档。

## 推荐同步范围

- 结构变化：更新 `docs/roadmap.md`
- agent 协作约定变化：更新 `docs/agents/README.md`
- 阶段性实现记录：在 `docs/agents/` 下新增对应笔记

## 当前约定

- 当前代码以“先做最小，但不把架构完全写死”为原则推进。
- `arch/` 放架构相关实现。
- `include/tianole/` 放尽量与具体架构无关的共享接口。
- 入口文件只负责串联启动流程，不承载文件加载、ELF 解析、内存映射统计等功能细节。
- 检查式日志应该放在对应功能模块中，避免把 `kernel_main()` 变成临时测试脚本。
- 内部 C API 不加 `tianole_` 前缀；这是单一内核代码库，不把项目名重复进函数名和类型名。
- 避免 `temp` / `tmp` 这类临时语义进入函数名、类型名和长期变量名；一次性局部变量可以用具体含义命名。
- C 代码风格参考 Linux：tabs 缩进，函数左花括号另起一行，`if/for/while` 左花括号留在行尾。
- 文本文件使用 LF 行尾。
- `.clang-format` 是当前格式化约定；如果环境有 `clang-format`，新增或大改 C/H 文件后应按它格式化。
- 根 `Makefile` 只做总控和通用规则；架构配置放在 `arch/<arch>/Makefile`，目录自己的源文件列表放在对应目录的 `Makefile` 中。
- 本地维护入口是 `scripts/check.sh`；GitHub Actions 应调用同一个脚本，避免 CI 逻辑和本地逻辑分叉。
