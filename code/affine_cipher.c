/* =====================================================================
 * affine_cipher.c —— 仿射密码（Affine Cipher）加解密完整实现（C 语言版）
 *
 * 实验名称：实验1 仿射变换加解密实验
 * 编译（Windows + MinGW/gcc 或 Linux 均可）：
 *     gcc -Wall -Wextra -O2 -g affine_cipher.c -o affine_cipher
 * 运行示例：
 *     affine_cipher.exe                      # 内置示例（加密、解密、字母映射表）
 *     affine_cipher.exe -e -a 5 -b 8 -t "affine cipher"   # 加密
 *     affine_cipher.exe -d -a 5 -b 8 -t "ihhwvc swfrcp"   # 解密
 *     affine_cipher.exe -T -a 5 -b 8         # 输出 26 个字母的加密映射表
 *     affine_cipher.exe -B -t "ihhwvc swfrcp"  # 暴力枚举全部 312 组密钥
 *     affine_cipher.exe -C -t "<英文密文>"      # 频率分析破译（无需已知密钥）
 *     affine_cipher.exe                      # 内置示例（加密、解密、字母映射表）
 *
 * 算法说明：
 *     加密：E(x) = (k1 * x + k2) mod 26
 *     解密：D(y) = k1^(-1) * (y - k2) mod 26
 *     其中 x、y 为字母在 [0,25] 中的编号（A/a=0，B/b=1，…，Z/z=25），
 *     k1 必须与 26 互素（gcd(k1,26)=1），否则乘法逆元 k1^(-1) 不存在，
 *     解密无法唯一还原；k2 为任意整数（模 26 使用）。
 * ===================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MOD       26        /* 英文字母表长度 */
#define MAX_TEXT  4096      /* 单次处理的文本最大长度（含结束符） */

/* 标准英文字母使用频率（百分比），用于频率分析破译，来源为通用英文语料统计结果 */
static const double ENGLISH_FREQ[MOD] = {
    8.167, 1.492, 2.782, 4.253, 12.702, 2.228, 2.015, 6.094, 6.966, 0.153, 0.772, 4.025,
    2.406, 6.749, 7.507, 1.929,  0.095, 5.987, 6.327, 9.056,  2.758, 0.978, 2.360, 0.150,
    1.974, 0.074
};


/* ==================== 一、基础数学函数 ==================== */

/* 求最大公约数（辗转相除法），返回值恒为非负 */
static int gcd_int(int a, int b)
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

/* 扩展欧几里得算法求乘法逆元：返回 x，使 a * x ≡ 1 (mod m)
 * 若 gcd(a,m) != 1（逆元不存在）则返回 -1 */
static int mod_inverse(int a, int m)
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

/* 判断密钥 k1 是否合法：必须与 26 互素（合法取值只有 12 个） */
static int is_valid_k1(int k1)
{
    int k = ((k1 % MOD) + MOD) % MOD;
    return gcd_int(k, MOD) == 1;
}

/* ==================== 二、单字符与字符串加解密 ==================== */

/* 加密一个字符：E(x) = (k1 * x + k2) mod 26
 * 大写字母仍为大写，小写字母仍为小写；
 * 数字、空格、标点以及中文等多字节字符一律原样保留 */
static char encrypt_char(char c, int k1, int k2)
{
    //下面的 % MOD + MOD) % MOD是为了处理负数取模
    if (c >= 'A' && c <= 'Z')
        return (char)('A' + ((k1 * (c - 'A') + k2) % MOD + MOD) % MOD);
    if (c >= 'a' && c <= 'z')
        return (char)('a' + ((k1 * (c - 'a') + k2) % MOD + MOD) % MOD);
    return c;
}

/* 解密一个字符：D(y) = k1^(-1) * (y - k2) mod 26，inv_k1 为预先算好的逆元 */
static char decrypt_char(char c, int k2, int inv_k1)
{
    if (c >= 'A' && c <= 'Z')
        return (char)('A' + ((inv_k1 * ((c - 'A') - k2)) % MOD + MOD) % MOD);
    if (c >= 'a' && c <= 'z')
        return (char)('a' + ((inv_k1 * ((c - 'a') - k2)) % MOD + MOD) % MOD);
    return c;
}

/* 加密整个字符串：逐字符加密，输出到调用者提供的缓冲区 */
static void affine_encrypt(const char *plain, char *cipher, int k1, int k2)
{
    size_t i;
    for (i = 0; plain[i] != '\0' && i < MAX_TEXT - 1; i++)
        cipher[i] = encrypt_char(plain[i], k1, k2);
    cipher[i] = '\0';
}

/* 解密整个字符串：k1 非法时返回 -1，正常返回 0 */
static int affine_decrypt(const char *cipher, char *plain, int k1, int k2)
{
    int inv_k1 = mod_inverse(k1, MOD);
    size_t i;

    if (inv_k1 < 0)
        return -1;                          /* k1 与 26 不互素，无法解密 */

    for (i = 0; cipher[i] != '\0' && i < MAX_TEXT - 1; i++)
        plain[i] = decrypt_char(cipher[i], k2, inv_k1);
    plain[i] = '\0';
    return 0;
}

/* ==================== 三、功能展示函数 ==================== */

/* 打印 26 个字母的加密映射表，便于手工核对 */
static void print_table(int k1, int k2)
{
    char plain_buf[MOD + 1], cipher_buf[MOD + 1];
    int inv = mod_inverse(k1, MOD);
    int i;

    for (i = 0; i < MOD; i++) {
        plain_buf[i]  = (char)('a' + i);
        cipher_buf[i] = encrypt_char((char)('a' + i), k1, k2);
    }
    plain_buf[MOD] = cipher_buf[MOD] = '\0';

    printf("明文(a-z) : %s\n", plain_buf);
    printf("密文(a-z) : %s\n", cipher_buf);
    printf("k1 = %d, k2 = %d, k1 的乘法逆元 k1^(-1) = %d (mod 26)\n", k1, k2, inv);
    printf("校验：%d * %d mod 26 = %d（应等于 1，说明逆元正确）\n",
           k1, inv, (k1 * inv) % MOD);
}

/* 输出一组加解密示例，并验证“解密(加密(明文)) == 明文” */
static int run_demo(void)
{
    const char *plain   = "Affine Cipher Test 2026, 仿射密码实验!";
    int k1 = 5, k2 = 8;
    char cipher[MAX_TEXT], back[MAX_TEXT];

    affine_encrypt(plain, cipher, k1, k2);
    if (affine_decrypt(cipher, back, k1, k2) != 0) {
        printf("解密失败：k1=%d 与 26 不互素。\n", k1);
        return 1;
    }

    printf("---------------- 加解密示例 ----------------\n");
    printf("密钥      : k1 = %d, k2 = %d\n", k1, k2);
    printf("明文      : %s\n", plain);
    printf("密文      : %s\n", cipher);
    printf("解密还原  : %s\n", back);
    printf("一致性校验: %s\n", (strcmp(plain, back) == 0) ? "通过（解密结果与明文完全相同）"
                                                            : "失败（解密结果与明文不同）");
    printf("\n---------------- 字母映射表 ----------------\n");
    print_table(k1, k2);
    return (strcmp(plain, back) == 0) ? 0 : 1;
}

/* ==================== 四、密码分析功能：暴力枚举与频率分析 ==================== */

/* 卡方统计量：衡量文本中各字母出现次数与标准英文频率的接近程度
 * 得分越小说明该文本越"像"正常的英文，可用于在 312 组候选密钥中挑出正确的那一组 */
static double score_english(const char *text)
{
    int counts[MOD] = {0};
    int total = 0, i;
    double score = 0.0;

    for (i = 0; text[i] != '\0'; i++) {
        if (text[i] >= 'a' && text[i] <= 'z') {
            counts[text[i] - 'a']++;
            total++;
        } else if (text[i] >= 'A' && text[i] <= 'Z') {
            counts[text[i] - 'A']++;
            total++;
        }
    }
    if (total == 0)
        return 1e18;                        /* 没有字母，认为完全不像英文 */

    for (i = 0; i < MOD; i++) {
        double expect = ENGLISH_FREQ[i] / 100.0 * total;    /* 该字母的期望出现次数 */
        double diff   = counts[i] - expect;
        score += diff * diff / expect;
    }
    return score;
}

/* 暴力枚举全部 312 组密钥（12 个合法 k1 × 26 个 k2），打印前 show_first 组候选明文
 * 返回实际枚举的密钥组数 */
static int brute_force(const char *cipher, int show_first)
{
    char buf[MAX_TEXT];
    int k1, k2, count = 0;

    for (k1 = 0; k1 < MOD; k1++) {
        if (!is_valid_k1(k1)) continue;
        for (k2 = 0; k2 < MOD; k2++) {
            affine_decrypt(cipher, buf, k1, k2);
            count++;
            if (count <= show_first)
                printf("  k1 = %2d, k2 = %2d → %s\n", k1, k2, buf);
        }
    }
    return count;
}

/* 频率分析破译（唯密文攻击）：遍历全部密钥，返回卡方得分最低的一组
 * 结果通过输出参数返回，便于其它功能复用 */
static void crack_by_frequency(const char *cipher, int *out_k1, int *out_k2,
                              char *out_plain, double *out_score)
{
    char buf[MAX_TEXT];
    int k1, k2, best_k1 = 0, best_k2 = 0, first = 1;
    double best = 0.0;

    for (k1 = 0; k1 < MOD; k1++) {
        if (!is_valid_k1(k1)) continue;
        for (k2 = 0; k2 < MOD; k2++) {
            double s;
            affine_decrypt(cipher, buf, k1, k2);
            s = score_english(buf);
            if (first || s < best) {
                first   = 0;
                best    = s;
                best_k1 = k1;
                best_k2 = k2;
                snprintf(out_plain, MAX_TEXT, "%s", buf);
            }
        }
    }
    *out_k1    = best_k1;
    *out_k2    = best_k2;
    *out_score = best;
}

/* 打印频率分析破译结果 */
static void print_crack(const char *cipher)
{
    char plain[MAX_TEXT];
    int k1, k2;
    double score;

    crack_by_frequency(cipher, &k1, &k2, plain, &score);
    printf("频率分析破译结果：k1 = %d, k2 = %d\n", k1, k2);
    printf("还原明文：%s\n", plain);
    printf("明文卡方得分（越小越像英文）：%.2f\n", score);
    printf("密文卡方得分（对照）：%.2f\n", score_english(cipher));
}

/* ==================== 五、命令行界面 ==================== */

/* 取路径中的文件名部分，仅用于帮助信息显示，例如 affine_cipher.exe */
static const char *base_name(const char *path)
{
    const char *p = path, *q = path;
    for (; *p != '\0'; p++)
        if (*p == '/' || *p == '\\') q = p + 1;
    return q;
}

static void print_usage(const char *prog)
{
    prog = base_name(prog);
    printf("仿射密码加解密程序（C 语言版）\n");
    printf("用法：\n");
    printf("  %s                                  运行内置示例（加密、解密、映射表）\n", prog);
    printf("  %s -e -a 5 -b 8 -t \"affine cipher\"   加密：E(x) = (5x + 8) mod 26\n", prog);
    printf("  %s -d -a 5 -b 8 -t \"ihhwvc swfrcp\"   解密：D(y) = 5^(-1)(y - 8) mod 26\n", prog);
    printf("  %s -T -a 5 -b 8                       输出 26 个字母的加密映射表\n", prog);
    printf("  %s -B -t \"ihhwvc swfrcp\"             暴力枚举全部 312 组密钥\n", prog);
    printf("  %s -C -t \"<英文密文>\"                 频率分析破译（无需已知密钥）\n", prog);
    printf("参数说明：-e 加密，-d 解密，-T 映射表，-B 暴力枚举，-C 频率分析，\n");
    printf("         -a 指定 k1，-b 指定 k2，-t 指定待处理文本\n");
    printf("         k1 必须与 26 互素，合法取值为 "
           "1,3,5,7,9,11,15,17,19,21,23,25\n");
}

int main(int argc, char *argv[])
{
    int encrypt_mode = -1;          /* 1=加密，0=解密，-1=未指定 */
    int k1 = -1, k2 = 0;
    const char *text = NULL;
    int show_table = 0, brute_mode = 0, crack_mode = 0;
    int i;
    char cipher[MAX_TEXT], plain[MAX_TEXT];

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-e") == 0) {
            encrypt_mode = 1;
        } else if (strcmp(argv[i], "-d") == 0) {
            encrypt_mode = 0;
        } else if (strcmp(argv[i], "-a") == 0 && i + 1 < argc) {
            k1 = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-b") == 0 && i + 1 < argc) {
            k2 = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            text = argv[++i];
        } else if (strcmp(argv[i], "-T") == 0) {
            show_table = 1;
        } else if (strcmp(argv[i], "-B") == 0 || strcmp(argv[i], "--brute") == 0) {
            brute_mode = 1;
        } else if (strcmp(argv[i], "-C") == 0 || strcmp(argv[i], "--crack") == 0) {
            crack_mode = 1;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            printf("未知参数：%s\n\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    if (brute_mode || crack_mode) {          /* 密码分析模式：不需要预先知道密钥 */
        if (text == NULL) {
            printf("错误：请用 -t 指定待分析的密文。\n");
            return 1;
        }
        if (brute_mode) {
            printf("共枚举 %d 组密钥（12 个合法 k1 × 26 个 k2），前 10 组结果：\n", 12 * MOD);
            brute_force(text, 10);
            printf("  ……（其余 %d 组略）\n", 12 * MOD - 10);
        } else {
            print_crack(text);
        }
        return 0;
    }

    if (show_table) {                        /* -T：只输出映射表 */
        if (k1 == -1) {
            printf("错误：输出映射表需要用 -a 指定 k1。\n");
            return 1;
        }
        if (!is_valid_k1(k1)) {
            printf("错误：k1 = %d 不合法，gcd(%d,26) = %d ≠ 1，乘法逆元不存在。\n",
                   k1, k1, gcd_int(((k1 % MOD) + MOD) % MOD, MOD));
            return 1;
        }
        print_table(k1, k2);
        return 0;
    }

    if (encrypt_mode == -1) {               /* 无参数：显示帮助并运行内置示例 */
        print_usage(argv[0]);
        printf("\n");
        return run_demo();
    }

    if (k1 == -1) {                          /* 未指定密钥 */
        printf("错误：请用 -a 指定 k1（必须与 26 互素）。\n");
        return 1;
    }
    if (!is_valid_k1(k1)) {
        printf("错误：k1 = %d 不合法，gcd(%d,26) = %d ≠ 1，"
               "乘法逆元不存在，无法解密。\n",
               k1, k1, gcd_int(((k1 % MOD) + MOD) % MOD, MOD));
        printf("合法 k1 取值为：1,3,5,7,9,11,15,17,19,21,23,25\n");
        return 1;
    }
    if (text == NULL) {
        printf("错误：请用 -t 指定待处理的文本。\n");
        return 1;
    }

    if (encrypt_mode == 1) {
        affine_encrypt(text, cipher, k1, k2);
        printf("明文：%s\n密文：%s\n", text, cipher);
    } else {
        if (affine_decrypt(text, plain, k1, k2) != 0) {
            printf("错误：k1 = %d 不合法，解密失败。\n", k1);
            return 1;
        }
        printf("密文：%s\n明文：%s\n", text, plain);
    }
    return 0;
}
