# 仿射密码（仿射变换）加解密

河北地质大学 · 信息工程学院 · 密码学课程实验 1

纯 C 实现的仿射密码加解密程序，无第三方依赖。除基础加解密外，还实现了**全部密钥空间暴力枚举**和**基于卡方统计的频率分析破译**——后者不需要知道密钥，只要密文足够长就能直接还原明文。

## 算法原理

```
加密：E(x) = (k1 · x + k2) mod 26
解密：D(y) = k1^(-1) · (y - k2) mod 26
```

`x`、`y` 是字母在 `[0, 25]` 中的编号（`A/a = 0`）。`k1` 必须与 26 互素，否则乘法逆元 `k1^(-1)` 不存在，解密无法唯一还原。26 以内与 26 互素的数只有 12 个，因此合法密钥共 `12 × 26 = 312` 组。

## 功能

| 功能 | 命令 | 说明 |
| --- | --- | --- |
| 加密 | `-e -a <k1> -b <k2> -t <明文>` | 输出密文 |
| 解密 | `-d -a <k1> -b <k2> -t <密文>` | 输出明文 |
| 字母映射表 | `-T -a <k1> -b <k2>` | 输出 26 个字母的加密对应表，并校验乘法逆元 |
| 暴力枚举 | `-B -t <密文>` | 遍历全部 312 组密钥，按英文似然度排序 |
| 频率分析破译 | `-C -t <密文>` | 卡方统计，无需已知密钥 |

不带参数直接运行会执行内置演示（加密 + 解密 + 字母映射表）。

## 编译运行

```bash
cd code
gcc -Wall -Wextra -O2 -g affine_cipher.c -o affine_cipher

./affine_cipher                                          # 内置演示
./affine_cipher -e -a 5 -b 8 -t "affine cipher"          # 加密 → ihhwvc swfrcp
./affine_cipher -d -a 5 -b 8 -t "ihhwvc swfrcp"          # 解密 → affine cipher
./affine_cipher -T -a 5 -b 8                             # 全部 26 个字母的映射表
./affine_cipher -B -t "ihhwvc swfrcp"                    # 暴力枚举 312 组密钥
./affine_cipher -C -t "<较长的英文密文>"                   # 频率分析破译
```

也可以用 Makefile：

```bash
make        # 编译
make run    # 编译并运行演示
make clean  # 删除可执行文件
```

Windows 下可双击 `code/编译并运行演示.bat` 一键编译运行。

## 实际运行效果

加密与解密（`k1 = 5, k2 = 8`）：

```
明文：affine cipher
密文：ihhwvc swfrcp
```

字母映射表（`-T`）：

```
明文(a-z) : abcdefghijklmnopqrstuvwxyz
密文(a-z) : insxchmrwbglqvafkpuzejotyd
k1 = 5, k2 = 8, k1 的乘法逆元 k1^(-1) = 21 (mod 26)
校验：5 * 21 mod 26 = 1（应等于 1，说明逆元正确）
```

频率分析破译：用 `k1 = 7, k2 = 5` 加密一段 571 字符的英文，只把密文交给 `-C`，不提供任何密钥信息：

```
频率分析破译结果：k1 = 7, k2 = 5
还原明文：cryptography is the practice and study of techniques for secure ...
明文卡方得分（越小越像英文）：87.97
密文卡方得分（对照）：6069.40
```

密钥还原完全正确。需要注意这只是单字母频率统计，**密文越短越不可靠**：几十个字符的短句可能收敛到错误密钥，这是统计方法本身的固有限制，而不是实现缺陷。

## 目录结构

```
├── code/
│   ├── affine_cipher.c          # 主程序（约 390 行）
│   ├── Makefile                 # make / make run / make clean
│   └── 编译并运行演示.bat         # Windows 一键编译并运行
├── images/
│   ├── 图1 仿射密码加解密算法流程图.png
│   ├── 生成流程图.py              # 绘制上述流程图的脚本
│   └── 3.py                      # 与 生成流程图.py 内容相同的副本
├── (0)affine_cipher.c代码解释.md    # 代码逐段讲解（架构、调用链、易错点）
├── 实验1 仿射变换加解密实验报告.md    # 实验报告正文
└── 运行说明与截图清单.md            # 复现步骤与截图清单
```

## 运行环境

Windows 11 / Linux 均可，需 gcc（本机验证版本：MinGW-W64 gcc 16.1.0），无第三方库依赖。

## 说明

本仓库只包含源代码与 Markdown 文档，不包含 Word 版报告、编译产物等文件。
