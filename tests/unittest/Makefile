LIB_NAME := libunittest.a

PREFIX := /usr/local

CC := gcc
CFLAGS := -Wall -Wextra -Wpedantic

BUILD_DIR := ./build
SRC_DIR := src

SRCS := $(shell find $(SRC_DIR) -name '*.c')
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

$(LIB_NAME): $(OBJS)
	ar rcs $@ $(OBJS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build/*

install: $(LIB_NAME)
	# Create necessary directories
	sudo mkdir -p $(PREFIX)/include/unittest
	sudo mkdir -p $(PREFIX)/lib
	# Copy the public header
	sudo cp include/unittest/unittest.h $(PREFIX)/include/unittest/
	# Copy the library file
	sudo cp $(LIB_NAME) $(PREFIX)/lib/

.PHONY: clean install
