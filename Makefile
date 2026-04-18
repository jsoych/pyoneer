TARGET  := pyoneer
SUBDIRS := vendor src
TOP_DIR := $(CURDIR)

# Build directories
PREFIX     := $(TOP_DIR)/build
PREFIX_BIN := $(TOP_DIR)/bin
PREFIX_LIB := $(PREFIX)/lib
PREFIX_INC := $(PREFIX)/include

export TOP_DIR PREFIX PREFIX_BIN PREFIX_LIB PREFIX_INC

# Toolchain
AR      := ar
ARFLAGS := rcs
CC      := gcc
CFLAGS  := -Wall -Wextra -Wpedantic -I$(PREFIX_INC)
LDFLAGS := -L$(PREFIX_LIB)
LIBS    := -lapi -lblueprint -lpyoneer -lshared

export AR ARFLAGS CC CFLAGS LIBS LDFLAGS

.PHONY: clean $(SUBDIRS)

$(PREFIX_BIN)/$(TARGET): prepare $(SUBDIRS)
	$(CC) $(CFLAGS) src/main.c $(LDFLAGS) $(LIBS) -o $@

prepare:
	@mkdir -p "$(PREFIX_BIN)" "$(PREFIX_LIB)" "$(PREFIX_INC)"

src: vendor

$(SUBDIRS):
	@$(MAKE) -C $@

clean:
	rm -rf "$(TOP_DIR)/build"
	rm -rf "$(TOP_DIR)/bin"
