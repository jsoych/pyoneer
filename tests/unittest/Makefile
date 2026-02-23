TARGET := example

CC := gcc
CFLAGS := -Wall -Wextra -Wpedantic

BUILD_DIR := ./build

SRCS := $(shell find . -name '*.c')
OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

example: $(OBJS)
	$(CC) $^ -o $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
