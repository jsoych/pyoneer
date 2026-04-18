SUBMODS := libmicrohttpd unittest

.PHONY: all $(SUBMODS) clean

all: $(SUBMODS)

libmicrohttpd:
	@echo "Configuring libmicrohttpd..."
	cd libmicrohttpd && [ -f configure ] || ./bootstrap
	# We "blank out" the exported LIBS and LDFLAGS for this command
	cd libmicrohttpd && [ -f Makefile ] || ./configure \
		--prefix="$(PREFIX)" \
		--disable-shared \
		--enable-static \
		--without-makeinfo \
		CFLAGS="-O2" \
		LDFLAGS="" \
		LIBS=""
	$(MAKE) -C libmicrohttpd install

unittest:
	@echo "Building unittest..."
	@mkdir -p "$(PREFIX_INC)/unittest" "$(PREFIX_LIB)"
	cp unittest/src/unittest.h "$(PREFIX_INC)/unittest/"
	$(MAKE) -C unittest
	cp unittest/libunittest.a "$(PREFIX_LIB)/"

clean:
	# Use -f to ignore errors if the sub-makefiles aren't generated yet
	-$(MAKE) -C libmicrohttpd clean
	-$(MAKE) -C unittest clean
