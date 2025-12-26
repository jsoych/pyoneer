INCLUDE := -Iapi -Iblueprints -Ipyoneers -Ishared

OBJS := $(patsubst %.o,$(BUILD_DIR)/%.o,$(OBJS))
OBJS += $(BUILD_DIR)/main.o

$(BIN_DIR)/$(TARGET): $(OBJS)
	$(CC) $^ -o $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@
