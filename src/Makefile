PACKAGES := api blueprint pyoneer shared

export AR ARFLAGS CC CFLAGS

export TOP_DIR PREFIX PREFIX_BIN PREFIX_LIB PREFIX_INC

.PHONY: all clean $(CLEAN) $(PACKAGES)

all: $(PACKAGES)

api: blueprint pyoneer shared

pyoneer: blueprint shared

blueprint: shared

$(PACKAGES):
	$(MAKE) -C $@
