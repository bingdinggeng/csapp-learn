# CS:APP Data Lab

本目录来自 CMU CS:APP 3e 官方自学版 Data Lab。作业答案位于 `bits.c`，共完成 13 道位级编程题。

## 验证命令

```bash
cd ~/csapp/datalab

# 检查是否违反运算符和语法限制
./dlc bits.c

# 显示每道题使用的运算符数量
./dlc -e bits.c

# 编译并运行正确性测试
make clean && make
./btest

# 运行完整自动评分
./driver.pl
```

当前自动评分结果为 `62/62`：正确性 `36/36`，操作数限制 `26/26`。

## 辅助工具

```bash
./ishow 0x80000000
./fshow 0x3f800000
```

`ishow` 用于查看整数的位表示，`fshow` 用于查看 IEEE 754 单精度浮点数的字段和值。

> 官方 Makefile 原本强制使用 `-m32`。本环境未预装 32 位 libc 开发头文件，因此已改为在 x86-64 下原生构建；Linux x86-64 的 `int` 和 `unsigned` 仍为 32 位，不影响本实验的题目语义和检查结果。
