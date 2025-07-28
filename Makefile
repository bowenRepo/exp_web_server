# ----------------------------------------------------
#          一个简单、安静且高效的 Makefile
# ----------------------------------------------------

# 编译器、编译选项和目标程序名
# 保持这部分是为了基本的灵活性
CC = gcc
CFLAGS = -Wall -g
TARGET = server

# 默认目标，让您可以直接运行 `make`
# .PHONY 确保 `all` 和 `clean` 总是可以被执行
.PHONY: all clean

all: $(TARGET)

# 核心编译规则
# 使用 '@' 来抑制命令回显，实现“静默”
# 使用 'echo' 来打印更友好的状态信息
$(TARGET): main.c
	@echo "  [CC]   编译: $^  ->  $@"
	@$(CC) $(CFLAGS) -o $@ $^

# 清理规则
clean:
	@echo "  [RM]   清理: $(TARGET)"
	@rm -f $(TARGET)