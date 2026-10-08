# Makefile —— 编译 C 语言版仿射密码程序
# 用法：make（编译）  make run（编译并运行演示与自检）  make clean（删除可执行文件）

CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -g
TARGET  = affine_cipher

all: $(TARGET)

$(TARGET): affine_cipher.c
	$(CC) $(CFLAGS) affine_cipher.c -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(TARGET).exe

.PHONY: all run clean
