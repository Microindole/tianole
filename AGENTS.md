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
