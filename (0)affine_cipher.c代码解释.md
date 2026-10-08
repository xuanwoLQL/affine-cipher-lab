# (0)affine_cipher.c——仿射密码加解密程序代码解释

主要代码存放位置
```
code/affine_cipher.c   ← 全程序唯一源文件（约 390 行，纯 C，无第三方依赖）
code/affine_cipher.exe ← 编译出的可执行文件
code/Makefile          ← 编译规则
code/编译并运行演示.bat  ← Windows 一键编译+运行脚本
```

整体架构 → 调用链 → 逐函数拆解 → 关键知识点 → 运行方式 → 易错点

## 1. 整体架构

一个用纯 C 写的**仿射密码（Affine Cipher）教学演示程序**，核心知识点是：

- **模运算**：`(a mod 26 + 26) mod 26` 处理负数取模，保证结果落在 `[0, 25]`
- **乘法逆元**：扩展欧几里得算法求 `k1⁻¹`，是"能解密"的前提
- **密码分析**：暴力枚举全部 312 组密钥 + 卡方统计频率分析破译

整个文件按注释分成五大模块：

| 模块 | 包含函数 | 作用 |
|---|---|---|
| 一、基础数学函数 | `gcd_int` / `mod_inverse` / `is_valid_k1` | 求最大公约数、乘法逆元、密钥合法性判断 |
| 二、单字符与字符串加解密 | `encrypt_char` / `decrypt_char` / `affine_encrypt` / `affine_decrypt` | 加解密核心，字符级处理、保留非字母 |
| 三、功能展示 | `print_table` / `run_demo` | 打印字母映射表、内置示例与一致性校验 |
| 四、密码分析 | `score_english` / `brute_force` / `crack_by_frequency` / `print_crack` | 暴力枚举、频率分析破译 |
| 五、命令行界面 | `base_name` / `print_usage` / `main` | 参数解析、帮助信息、功能分发 |

一句话总结：**算法本身只靠 4 个函数（加密 2 个、解密 2 个），其余都是"怎么把功能展示给用户"的壳。**

## 2. 调用链全景图（main 是怎么分发功能的）

```
main(argc, argv)                ← 逐参数解析 -e/-d/-a/-b/-t/-T/-B/-C/-h
  │
  ├─ 有 -B（暴力枚举）─────────────► brute_force()
  │                                   └─ 对每个 (k1,k2) 调affine_decrypt()
  │
  ├─ 有 -C（频率分析）─────────────► print_crack()
  │                                   └─ crack_by_frequency()
  │                                        ├─ affine_decrypt()  遍历 312 组密钥
  │                                        └─ score_english()   每组算卡方得分
  │
  ├─ 有 -T（映射表）──────────────► print_table()
  │                                   ├─ encrypt_char()  逐个字母加密
  │                                   └─ mod_inverse()   顺便打印逆元并校验
  │
  ├─ 无任何参数 ──────────────────► run_demo()（内置示例）
  │                                   ├─ affine_encrypt() → affine_decrypt()
  │                                   ├─ strcmp 一致性校验
  │                                   └─ print_table()
  │
  └─ 有 -e / -d（普通加解密）──────► affine_encrypt()  或  affine_decrypt()
                                        │                     │
                                        └─ encrypt_char()     ├─ mod_inverse()（先算逆元）
                                                              └─ decrypt_char()

加密数学式：  E(x) = (k1 * x + k2) mod 26
解密数学式：  D(y) = k1⁻¹ * (y - k2) mod 26
字母编号：    A/a=0，B/b=1，…，Z/z=25
```

## 3. 逐块拆解

### 3.1 宏与常量（第 28~36 行）

```
第28行
#define MOD       26        /* 英文字母表长度 */
#define MAX_TEXT  4096      /* 单次处理的文本最大长度（含结束符） */
```
- `MOD = 26`：整个程序的"模"，所有取模运算都用它。改成 27 就变成支持空格+26 字母的版本——**算法骨架不变，改模长即可扩展**。
- `MAX_TEXT = 4096`：缓冲区上限，防止溢出。字符串函数（`affine_encrypt` 等）都带 `i < MAX_TEXT - 1` 这个哨兵。

```
第32行
static const double ENGLISH_FREQ[MOD] = {
    8.167, 1.492, ... 0.074   /* 26 个字母的标准英文频率（百分比） */
};
```
- 这是**破译用的"标准答案"**：统计英文语料里每个字母出现的百分比。`e` 最高（12.702%），`z` 最低（0.074%）。
- `static const`：只在本文件可见 + 只读，防止被意外修改。

### 3.2 一、基础数学函数（第 39~76 行）

```
第42行
static int gcd_int(int a, int b)   /* 辗转相除法求最大公约数 */
{
    if (a < 0) a = -a;
    if (b < 0) b = -b;
    while (b != 0) {
        int t = a % b;
        a = b;
        b = t;
    }
    return a;
}
```
- 先取绝对值，保证返回值恒非负。
- **为什么需要 gcd？** 仿射密码要求 `gcd(k1, 26) = 1`（互素）。只有互素时 `k1` 在模 26 下才有乘法逆元，解密才能唯一还原。
- 辗转相除法：`gcd(a,b) = gcd(b, a%b)`，反复取余直到余数为 0，剩下的就是最大公约数。

```
第56行
static int mod_inverse(int a, int m)  /* 扩展欧几里得求乘法逆元 */
{
    int t = 0, newt = 1;
    int r = m, newr = ((a % m) + m) % m;   /* 把 a 归一到 [0, m) */

    while (newr != 0) {
        int q = r / newr;
        int tmp = t - q * newt;  t = newt;  newt = tmp;
        tmp     = r - q * newr;  r = newr;  newr = tmp;
    }
    if (r > 1)      return -1;             /* 不互素，无逆元 */
    if (t < 0)      t += m;                /* 归一化到 [0, m) */
    return t;
}
```
- 求的是满足 `a * x ≡ 1 (mod m)` 的 `x`。
- `newr = ((a % m) + m) % m`：**C 语言里负数取模结果是负数**（如 `-3 % 26 = -3`），所以先归一到 `[0, m)`，这就是注释里"处理负数取模"的套路，全程序反复使用。
- 返回 `-1` 表示 `gcd(a,m) ≠ 1`（比如 `a=2, m=26`），此时逆元不存在——`affine_decrypt` 靠这个返回值判断"能不能解密"。
- **为什么解密一定要逆元？** 加密是 `y = k1·x + k2`，要还原 `x` 必须两边同乘 `k1⁻¹`。没有逆元，等式解不出来。

```
第72行
static int is_valid_k1(int k1)   /* 判断 k1 是否与 26 互素 */
{
    int k = ((k1 % MOD) + MOD) % MOD;
    return gcd_int(k, MOD) == 1;
}
```
- 把 k1 归一到 `[0,26)` 再判断。合法取值只有 12 个：`1,3,5,7,9,11,15,17,19,21,23,25`（帮助信息里也打印了）。
- 程序里凡是"解密前"都要先过这一关（`main` 里有两处）。

### 3.3 二、单字符与字符串加解密（第 78~125 行）—— 算法核心

```
第83行
static char encrypt_char(char c, int k1, int k2)   /* E(x) = (k1*x + k2) mod 26 */
{
    //下面的 % MOD + MOD) % MOD是为了处理负数取模
    if (c >= 'A' && c <= 'Z')
        return (char)('A' + ((k1 * (c - 'A') + k2) % MOD + MOD) % MOD);
    if (c >= 'a' && c <= 'z')
        return (char)('a' + ((k1 * (c - 'a') + k2) % MOD + MOD) % MOD);
    return c;
}
```
逐行拆：
- `c - 'A'`：把字母转成编号（`'A'→0`）。用 ASCII 连续编码的特性，`'B'-'A' = 1`。
- `(k1 * 编号 + k2) % MOD`：套加密公式。
- **`+ MOD) % MOD` 的作用**：当 `k1*编号 + k2` 是负数时（k2 可为负），C 的 `%` 会得负数，加一个 MOD 再取模就保证结果落在 `[0,25]`。这就是那句注释说的"处理负数取模"。
- `'A' + 结果`：把编号翻译回字母。
- 大写、小写分开处理，**非字母（数字、空格、标点、中文）原样返回**——所以它能加密带中文的字符串而不会乱码。

```
第94行
static char decrypt_char(char c, int k2, int inv_k1)  /* D(y) = k1⁻¹*(y - k2) mod 26 */
{
    if (c >= 'A' && c <= 'Z')
        return (char)('A' + ((inv_k1 * ((c - 'A') - k2)) % MOD + MOD) % MOD);
    ...
}
```
- 注意解密函数**不接收 k1，接收的是提前算好的 `inv_k1`**（乘法逆元）。因为逆元只算一次就够了，每个字符解密都复用，避免重复计算。
- 公式：先减 k2，再乘逆元，最后取模。

```
第104行
static void affine_encrypt(const char *plain, char *cipher, int k1, int k2)
{
    size_t i;
    for (i = 0; plain[i] != '\0' && i < MAX_TEXT - 1; i++)
        cipher[i] = encrypt_char(plain[i], k1, k2);
    cipher[i] = '\0';
}
```
- 逐字符加密整串。**`i < MAX_TEXT - 1` 是防溢出保险**：即使明文超长，也只写到缓冲区最后一个位置，然后手动补 `'\0'`。

```
第113行
static int affine_decrypt(const char *cipher, char *plain, int k1, int k2)
{
    int inv_k1 = mod_inverse(k1, MOD);
    size_t i;

    if (inv_k1 < 0)
        return -1;     /* k1 与 26 不互素，无法解密 */

    for (i = 0; cipher[i] != '\0' && i < MAX_TEXT - 1; i++)
        plain[i] = decrypt_char(cipher[i], k2, inv_k1);
    plain[i] = '\0';
    return 0;
}
```
- 返回值设计：`-1` 表示密钥非法解密失败，`0` 表示成功。调用方（`main`、`run_demo`、`brute_force`、`crack_by_frequency`）都依赖这个返回值判断。
- **先算逆元、失败提前返回**——不用先把整串解密完才发现逆元不存在。

### 3.4 三、功能展示（第 127~172 行）

```
第130行
static void print_table(int k1, int k2)
```
- 打印 `a~z` 的明文行和对应密文行，方便手工核对映射是否正确。
- 顺带打印 `k1 * k1⁻¹ mod 26`，**校验逆元算得对不对**（应该等于 1）。

```
第150行
static int run_demo(void)
```
- 内置示例：明文 `"Affine Cipher Test 2026, 仿射密码实验!"`，密钥 `k1=5, k2=8`。
- 加密 → 解密 → `strcmp` 对比，输出"一致性校验：通过/失败"。**这是整个程序的自动自测**：跑一遍就知道算法有没有写对。
- 明文里故意混了数字、逗号、中文，用来演示"非字母原样保留"。

### 3.5 四、密码分析（第 174~264 行）

```
第178行
static double score_english(const char *text)   /* 卡方统计量 */
{
    ...
    for (i = 0; i < MOD; i++) {
        double expect = ENGLISH_FREQ[i] / 100.0 * total;
        double diff   = counts[i] - expect;
        score += diff * diff / expect;
    }
    return score;
}
```
- 统计文本里 26 个字母各出现多少次，再和标准英文频率对比。
- 公式是卡方检验：`Σ (观测值 - 期望值)² / 期望值`。**得分越小，文本越"像"正常英文**。
- 没有字母（`total == 0`）直接返回 `1e18`（一个天文数字），表示"完全不像英文"。
- 破译思路：错误的密钥解出的是乱码，字母分布接近均匀，卡方得分高；**正确的密钥解出的是正常英文，得分断崖式最低**。

```
第206行
static int brute_force(const char *cipher, int show_first)
```
- 两层循环穷举：12 个合法 k1 × 26 个 k2 = 312 组。
- `is_valid_k1(k1)` 跳过非法 k1——**不跳的话 gcd 不互素，`affine_decrypt` 会返回 -1 解不出来**。
- 每组密钥都尝试解密并打印（只打印前 `show_first` 组，默认 10 组），人眼挑出像英文的那行。

```
第225行
static void crack_by_frequency(const char *cipher, int *out_k1, int *out_k2,
                               char *out_plain, double *out_score)
```
- 同样遍历 312 组密钥，但用 `score_english` 自动打分，记录**得分最低**的一组作为结果。
- **结果用输出参数返回**（`*out_k1` 等），而不是直接打印——这样别的功能也能复用这套破译逻辑。`print_crack` 就是它的"打印壳"。
- 这属于**唯密文攻击**：只需要密文，不需要知道密钥。

### 3.6 五、命令行界面（第 266~392 行）

```
第269行
static const char *base_name(const char *path)   /* 取路径里的文件名 */
```
- 遍历路径字符串，记住最后一个 `/` 或 `\` 后面的位置，用于帮助信息里只显示 `affine_cipher.exe` 而不是完整路径。

```
第294行
int main(int argc, char *argv[])
```
解析逻辑：
- `-e` 加密 / `-d` 解密：`encrypt_mode = 1 / 0`
- `-a k1` / `-b k2` / `-t text`：取参数值（`atoi` 转数字）
- `-T` 映射表 / `-B` 暴力枚举 / `-C` 频率分析 / `-h` 帮助
- **未知参数直接报错 + 打印用法并 return 1**——参数解析要"fail fast"。

分发逻辑（按优先级）：
1. `brute_mode || crack_mode` → 密码分析模式（此时不需要 k1/k2，但必须有 `-t` 密文）
2. `show_table` → 映射表（必须有 `-a`，且 k1 必须合法）
3. `encrypt_mode == -1`（没有任何加解密参数）→ 显示帮助 + 跑内置示例
4. 普通加解密：先检查 k1 是否指定、是否合法、是否有文本，再加密/解密

**注意错误处理的顺序**：每个分支都用 `return 1` 提前退出，绝不"带着错误参数继续往下走"。

## 4. 关键知识点速查表

| 知识点 | 出现位置 | 一句话 |
|---|---|---|
| 负数取模 | `encrypt_char` / `mod_inverse` 等 | C 的 `%` 对负数得负值，用 `(x % MOD + MOD) % MOD` 归一到 `[0, MOD)` |
| 互素条件 | `is_valid_k1` | `gcd(k1,26) ≠ 1` 则逆元不存在、无法解密，合法 k1 只有 12 个 |
| 辗转相除法 | `gcd_int` | `gcd(a,b) = gcd(b, a%b)`，反复取余 |
| 扩展欧几里得 | `mod_inverse` | 求 `a·x ≡ 1 (mod m)` 的 x；不互素返回 -1 |
| 逆元提前计算 | `affine_decrypt` | `inv_k1` 只算一次，全串解密复用，避免每字符重算 |
| ASCII 连续编码 | `encrypt_char` | `c - 'A'` 把字母转编号，`'A' + n` 把编号转字母 |
| 非字母原样保留 | `encrypt_char` | 只处理 `A-Z` / `a-z`，数字标点中文不动 |
| 缓冲区防溢出 | `affine_encrypt` 等 | 循环带 `i < MAX_TEXT - 1` 哨兵 + 手动补 `'\0'` |
| 卡方检验 | `score_english` | `Σ(观测-期望)²/期望`，得分越低越像英文 |
| 唯密文攻击 | `crack_by_frequency` | 只需密文，遍历密钥 + 自动打分挑出最像英文的 |
| 输出参数返回 | `crack_by_frequency` | 结果写进 `*out_k1` 等指针，函数不负责打印，便于复用 |
| fail fast | `main` | 参数缺失/非法立刻 `return 1`，不带着错误状态继续 |

## 5. 运行方式

```
gcc -Wall -Wextra -O2 -g affine_cipher.c -o affine_cipher   # 编译

affine_cipher.exe                        # 无参数：帮助 + 内置示例（自动自测）
affine_cipher.exe -e -a 5 -b 8 -t "affine cipher"   # 加密
affine_cipher.exe -d -a 5 -b 8 -t "ihhwvc swfrcp"   # 解密
affine_cipher.exe -T -a 5 -b 8            # 输出 26 字母加密映射表
affine_cipher.exe -B -t "ihhwvc swfrcp"   # 暴力枚举全部 312 组密钥
affine_cipher.exe -C -t "<英文密文>"       # 频率分析破译（无需密钥）
```
Windows 下也可以直接双击 `code/编译并运行演示.bat`，或 `make`（有 Makefile）。

## 6. 易错点 / 设计要点（写这类代码最容易踩的坑）

| 坑 | 现象 | 根因与解法 |
|---|---|---|
| k1 不互素就解密 | 解出来全是乱码 / 程序报"逆元不存在" | 忘了 `gcd(k1,26)=1` 的前提；本程序在 `is_valid_k1` + `mod_inverse` 双重拦截 |
| 负数取模 | `-3 % 26` 在 C 里得 `-3`，结果数组越界/乱码 | 必须 `(x % MOD + MOD) % MOD`，代码里注释专门标了这句 |
| 逆元重复计算 | 性能浪费（虽然这里影响不大） | 解密时先算一次 `inv_k1`，字符循环里只传 `inv_k1` |
| 缓冲区溢出 | 超长明文破坏内存 | 所有串处理循环都带 `MAX_TEXT - 1` 哨兵 |
| 忘记补 `'\0'` | 打印明文/密文时后面跟着乱码 | 循环结束后手动 `buf[i] = '\0'` |
| 解密返回值被忽略 | 密钥非法时程序还在继续 | `affine_decrypt` 返回 -1/0，`main` 和 `run_demo` 都检查了 |

> 一句话理解整个程序：**数学部分（逆元+加解密）只有约 50 行，其余 340 行都在做"怎么把 312 组密钥试一遍、怎么把结果讲给人听"**——这也正是实验报告的"加解密功能 + 破译功能"两条主线。
