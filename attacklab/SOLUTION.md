# CS:APP Attack Lab 答案

本目录对应 CS:APP 第三个实验 Attack Lab，目标文件的 cookie 为
`0x59b997fa`。

## 文件说明

- `result1.txt`：Phase 1，跳转到 `touch1`。
- `result2.txt`：Phase 2，代码注入并调用 `touch2(cookie)`。
- `result3.txt`：Phase 3，代码注入并调用 `touch3(cookie_string)`。
- `result4.txt`：Phase 4，使用 ROP 调用 `touch2(cookie)`。
- `result5.txt`：Phase 5，使用 ROP 调用 `touch3(cookie_string)`。
- `inject2.s`、`inject3.s`：Phase 2 和 Phase 3 的注入汇编。
- `verify.sh`：依次验证全部五个阶段。

## 使用方法

在 WSL 中运行单关：

```bash
cd /root/csapp/attacklab
./hex2raw < result1.txt | ./ctarget -q
./hex2raw < result4.txt | ./rtarget -q
```

验证全部答案：

```bash
cd /root/csapp/attacklab
bash verify.sh
```

`-q` 会禁止程序连接课程的在线评分服务器，适合本地自学。
