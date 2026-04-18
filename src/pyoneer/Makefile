# Package name
PACKAGE := $(notdir $(CURDIR))

# Toolchain
CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -Wpedantic -I../../build/include
AR      ?= ar
ARFLAGS ?= rcs

# Staging directories
PREFIX_INC ?= ../../build/include
PREFIX_LIB ?= ../../build/lib

# Automatic file discovery
SRCS    := $(wildcard *.c)
HDRS    := $(wildcard *.h)
OBJS    := $(SRCS:.c=.o)
TARGET  := $(PREFIX_LIB)/lib$(PACKAGE).a

.PHONY: all clean

all: $(TARGET)

# Archive object files into a static library
$(TARGET): $(OBJS)
	@mkdir -p $(PREFIX_LIB) $(PREFIX_INC)/$(PACKAGE)
	$(AR) $(ARFLAGS) $@ $^
	cp $(HDRS) $(PREFIX_INC)/$(PACKAGE)/

# Pattern rule for objects
%.o: %.c
	$(CC) $(CFLAGS) -I$(PREFIX_INC) -c $< -o $@

clean:
	rm -f *.o $(TARGET)
	rm -rf $(PREFIX_INC)/$(PACKAGE)
