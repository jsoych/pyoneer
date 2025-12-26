TARGET := pyoneer

CC := gcc
CFLAGS := -Wall -Wextra -Wpedantic

BIN_DIR := ../bin
BUILD_DIR := ../build
SRC_DIR := ../src

SRCS := $(shell find src -name '*.c' -depth 2)
OBJS := $(patsubst src/%.c,%.o,$(SRCS))

export CC CFLAGS BIN_DIR BUILD_DIR OBJS SRC_DIR TARGET

pyoneer:
	@$(MAKE) -C src

tests:
	@$(MAKE) -C tests

clean:
	rm -rf bin/pyoneer build/*

.PHONY: clean tests pyoneer
