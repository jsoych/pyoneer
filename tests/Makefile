MODULES := $(shell find $(SRC_DIR) -type d -depth 1)
INCLUDE := $(patsubst %,-I%,$(MODULES))
INCLUDE += -Iapi -Iblueprints -Ipyoneers -Ishared

API_SRCS := $(shell find api -name '*.c')
API_OBJS := $(patsubst %.c,$(BUILD_DIR)/tests/%.o,$(API_SRCS))

BLUE_SRCS := $(shell find blueprints -name '*.c')
BLUE_OBJS := $(patsubst %.c,$(BUILD_DIR)/tests/%.o,$(BLUE_SRCS))

PYONEER_SRCS := $(shell find pyoneers -name '*.c')
PYONEER_OBJS := $(patsubst %.c,$(BUILD_DIR)/tests/%.o,$(PYONEER_SRCS))

SHARED_SRCS := $(shell find shared -name '*.c')
SHARED_OBJS := $(patsubst %.c,$(BUILD_DIR)/tests/%.o,$(SHARED_SRCS))

UNITTEST_SRCS := $(shell find shared -name '*.c')
UNITTEST_OBJS := $(patsubst %.c,$(BUILD_DIR)/tests/%.o,$(UNITTEST_SRCS))

OBJS := $(patsubst %.o,$(BUILD_DIR)/%.o,$(OBJS))

all: $(BIN_DIR)/test_api $(BIN_DIR)/test_blueprints $(BIN_DIR)/test_pyoneers

$(BIN_DIR)/test_api: $(OBJS) $(API_OBJS) $(SHARED_OBJS)
	$(CC) $^ -o $@

$(BIN_DIR)/test_blueprints: $(OBJS) $(BLUE_OBJS) $(SHARED_OBJS)
	$(CC) $^ -o $@

$(BIN_DIR)/test_pyoneers: $(OBJS) $(PYONEER_OBJS) $(SHARED_OBJS)
	$(CC) $^ -o $@

$(BUILD_DIR)/tests/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

.PHONY: all $(OBJS)
