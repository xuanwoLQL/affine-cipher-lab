# -*- coding: utf-8 -*-
"""生成《实验1 仿射变换加解密实验报告》所用的算法流程图（真实绘制，非截图）。

说明：
  1. 文字统一使用 SimHei / 微软雅黑，因此框内文字不使用 ⁻¹、′ 等 SimHei 缺失的
     特殊符号，改用 ^(-1)、k1^(-1) 这类 ASCII 写法，避免出现方块乱码；
  2. 画布右侧预留 75 像素空白，"验证失败重试" 的红色回环线走 x = 1045，
     保证任何连线和箭头都不会穿过图形内部；
  3. 默认把图片保存到脚本所在目录（可用参数指定其它目录）。
"""

import math
import os
import sys

from PIL import Image, ImageDraw, ImageFont


def pick_font(*candidates):
    """按顺序挑选第一个存在的字体文件，避免某个字体缺失时脚本直接报错。"""
    for path in candidates:
        if os.path.exists(path):
            return path
    raise SystemExit("未找到可用中文字体：" + "、".join(candidates))


FONT_PATH = pick_font(r"C:\Windows\Fonts\simhei.ttf", r"C:\Windows\Fonts\simsun.ttc")
FONT_BOLD = pick_font(r"C:\Windows\Fonts\msyhbd.ttc", r"C:\Windows\Fonts\msyh.ttc",
                      r"C:\Windows\Fonts\simhei.ttf")

# 画布高度 1650、宽度 1120：右侧留白用于“验证失败重试”的回环连线
W, H = 1120, 1650
BG = "white"
PROC_FILL, PROC_LINE = "#E8F1FB", "#2F6FB2"
DEC_FILL, DEC_LINE = "#FFF6E0", "#D9A03A"
TERM_FILL, TERM_LINE = "#E7F6EA", "#3E9E58"
ERR_FILL, ERR_LINE = "#FDECEC", "#C0504D"
TEXT = "#1F1F1F"

RETRY_X = 1045          # 验证失败重试回环连线的横坐标（在所有图形右侧）


def font(size, bold=False):
    return ImageFont.truetype(FONT_BOLD if bold else FONT_PATH, size)


F_TITLE = font(30, bold=True)
F_BOX = font(20)
F_SMALL = font(18)


def wrap(draw, text, fnt, max_w):
    """按宽度对中文文本做简单换行。"""
    lines, cur = [], ""
    for ch in text:
        if ch == "\n":
            lines.append(cur)
            cur = ""
            continue
        if draw.textlength(cur + ch, font=fnt) > max_w and cur:
            lines.append(cur)
            cur = ch
        else:
            cur += ch
    if cur:
        lines.append(cur)
    return "\n".join(lines)


def process(draw, cx, cy, w, h, text, fnt=F_BOX, fill=PROC_FILL, line=PROC_LINE, radius=10):
    draw.rounded_rectangle([cx - w / 2, cy - h / 2, cx + w / 2, cy + h / 2],
                           radius=radius, fill=fill, outline=line, width=3)
    draw.multiline_text((cx, cy), wrap(draw, text, fnt, w - 30), font=fnt, fill=TEXT,
                        anchor="mm", align="center", spacing=6)


def diamond(draw, cx, cy, w, h, text, fnt=F_BOX):
    draw.polygon([(cx, cy - h / 2), (cx + w / 2, cy), (cx, cy + h / 2), (cx - w / 2, cy)],
                 fill=DEC_FILL, outline=DEC_LINE, width=3)
    draw.multiline_text((cx, cy), wrap(draw, text, fnt, w * 0.52), font=fnt, fill=TEXT,
                        anchor="mm", align="center", spacing=6)


def terminal(draw, cx, cy, w, h, text, fnt=F_BOX):
    draw.ellipse([cx - w / 2, cy - h / 2, cx + w / 2, cy + h / 2],
                 fill=TERM_FILL, outline=TERM_LINE, width=3)
    draw.multiline_text((cx, cy), text, font=fnt, fill=TEXT, anchor="mm", align="center")


def arrow(draw, p1, p2, color="#333333", width=3):
    x1, y1 = p1
    x2, y2 = p2
    draw.line([x1, y1, x2, y2], fill=color, width=width)
    ang = math.atan2(y2 - y1, x2 - x1)
    size = 12
    pts = [(x2, y2),
           (x2 - size * math.cos(ang - 0.4), y2 - size * math.sin(ang - 0.4)),
           (x2 - size * math.cos(ang + 0.4), y2 - size * math.sin(ang + 0.4))]
    draw.polygon(pts, fill=color)


img = Image.new("RGB", (W, H), BG)
d = ImageDraw.Draw(img)

d.text((W / 2 - 20, 40), "仿射密码加解密算法流程图", font=F_TITLE, fill="#1F3A5F", anchor="mm")

CX = 430                      # 主流程中心线
T = 90                        # 标题下偏移

# ========== 节点 ==========
terminal(d, CX, T + 60, 200, 56, "开始")                                     # 开始
process(d, CX, T + 140, 560, 50, "输入明文 P")                               # 输入明文
process(d, CX, T + 210, 560, 50, "输入密钥 k1 和 k2")                        # 输入密钥
diamond(d, CX, T + 320, 420, 130, "gcd(k1, 26) = 1 ?")                       # 判断密钥合法性
process(d, 830, T + 320, 330, 90, "提示 k1 不合法，\n要求重新输入密钥",
        fill=ERR_FILL, line=ERR_LINE, fnt=F_SMALL)
process(d, CX, T + 440, 420, 56, "取明文中的下一个字符 c")                     # 取字符
diamond(d, CX, T + 545, 400, 120, "c 是英文字母 ?")                           # 判断是否字母（上移 25px）
process(d, 830, T + 545, 330, 80, "该字符原样保留", fnt=F_SMALL)              # 原样保留（同步上移）
process(d, CX, T + 690, 620, 86,
        "x = c 在字母表中的编号\ny = (k1 * x + k2) mod 26\nc' = 编号 y 对应的大小写字母")
diamond(d, CX, T + 830, 420, 120, "明文已全部处理完 ?")                        # 是否处理完
process(d, CX, T + 940, 480, 56, "输出密文 C")                               # 输出密文
process(d, CX, T + 1030, 560, 56, "用扩展欧几里得算法计算 k1^(-1) mod 26")     # 求逆元
process(d, CX, T + 1120, 660, 56, "解密：x = k1^(-1) * (y - k2) mod 26，还原每个字母")
process(d, CX, T + 1210, 480, 56, "输出解密结果 P'")                          # 输出明文
diamond(d, CX, T + 1350, 420, 130, "P' 与 P 完全一致 ?")                      # 校验
process(d, 830, T + 1350, 330, 90, "提示错误：\n检查密钥与字符处理",
        fill=ERR_FILL, line=ERR_LINE, fnt=F_SMALL)
terminal(d, CX, T + 1480, 200, 56, "结束")                                   # 结束

# ========== 连线 ==========
arrow(d, (CX, T + 88), (CX, T + 115))                                  # 开始 -> 输入明文
arrow(d, (CX, T + 165), (CX, T + 185))                                 # 输入明文 -> 输入密钥
arrow(d, (CX, T + 235), (CX, T + 255))                                 # 输入密钥 -> 判断1

# 密钥合法性判断：否 -> 错误提示 -> 回到输入密钥
arrow(d, (CX + 210, T + 320), (830 - 165, T + 320))
d.line([831, T + 365, 831, T + 400, 90, T + 400, 90, T + 210], fill="#C0504D", width=3)
arrow(d, (90, T + 210), (CX - 280, T + 210), color="#C0504D")

arrow(d, (CX, T + 385), (CX, T + 412))                                 # 是 -> 取字符
arrow(d, (CX, T + 468), (CX, T + 485))                                 # 取字符 -> 判断2（终点上移）

# 字符判断：否 -> 原样保留 -> 汇合到主流程
arrow(d, (CX + 200, T + 545), (830 - 165, T + 545))                    # 分支横线上移
d.line([830, T + 585, 830, T + 750, CX + 6, T + 750], fill="#333333", width=3)  # 起点上移
arrow(d, (830, T + 750), (CX + 6, T + 750))
arrow(d, (CX, T + 605), (CX, T + 647))                                 # 是 -> 加密（起点上移）
arrow(d, (CX, T + 733), (CX, T + 770))                                 # 加密 -> 判断3

# 未处理完：回到取字符
arrow(d, (CX - 210, T + 830), (60, T + 830))
d.line([60, T + 830, 60, T + 440], fill="#333333", width=3)
arrow(d, (60, T + 440), (CX - 210, T + 440))

# 处理完 -> 输出密文 -> 求逆元 -> 解密 -> 输出明文 -> 校验
arrow(d, (CX, T + 890), (CX, T + 912))
arrow(d, (CX, T + 968), (CX, T + 1002))
arrow(d, (CX, T + 1058), (CX, T + 1092))
arrow(d, (CX, T + 1148), (CX, T + 1182))
arrow(d, (CX, T + 1238), (CX, T + 1285))

# 校验一致性分支
arrow(d, (CX, T + 1415), (CX, T + 1452))                               # 一致 -> 结束
arrow(d, (CX + 210, T + 1350), (830 - 165, T + 1350))                  # 否 -> 错误提示

# 【新增】解密验证失败后，绕回重新输入密钥的连线
d.line([830, T + 1395, 830, T + 1430, 960, T + 1430, 960, T + 210], fill="#C0504D", width=3)
arrow(d, (960, T + 210), (CX + 280, T + 210), color="#C0504D", width=3)

# ========== 分支标签 ==========
d.text((CX + 214, T + 302), "否", font=F_SMALL, fill="#C0504D", anchor="lm")
d.text((CX - 14, T + 400), "是", font=F_SMALL, fill="#3E9E58", anchor="rm")
d.text((CX + 214, T + 525), "否", font=F_SMALL, fill="#C0504D", anchor="lm")   # 上移
d.text((CX - 14, T + 625), "是", font=F_SMALL, fill="#3E9E58", anchor="rm")    # 上移
d.text((CX - 230, T + 795), "否", font=F_SMALL, fill="#C0504D", anchor="rm")
d.text((CX + 14, T + 900), "是", font=F_SMALL, fill="#3E9E58", anchor="lm")
d.text((CX + 214, T + 1312), "否", font=F_SMALL, fill="#C0504D", anchor="lm")
d.text((CX + 14, T + 1420), "是", font=F_SMALL, fill="#3E9E58", anchor="lm")
d.text((965, T + 800), "验证失败重试", font=F_SMALL, fill="#C0504D", anchor="lm")

# ========== 保存 ==========
out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.dirname(os.path.abspath(__file__))
os.makedirs(out_dir, exist_ok=True)
out = os.path.join(out_dir, "3.png")
img.save(out, "PNG")
print("已保存：", out, img.size)