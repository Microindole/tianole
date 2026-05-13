# Tianole

Tianole 是一个学习用的自制操作系统项目，当前目标平台是 `UEFI x86_64`。

## 运行

```bash
make all
make run
```

无界面运行并查看日志：

```bash
make run-headless
cat build/debug.log
```

完整检查：

```bash
scripts/check.sh
```

## 文档

- [AGENTS.md](AGENTS.md)：agent 接手入口
- [docs/agents/tasks/README.md](docs/agents/tasks/README.md)：当前任务阶段
- [docs/roadmap.md](docs/roadmap.md)：路线记录
