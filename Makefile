CC ?= gcc
LD := $(CC)

PREFIX ?= /usr/local

uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')

CFLAGS += -Iinclude
CFLAGS += -O3
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Wshadow
CFLAGS += -Wswitch-default
CFLAGS += -Wswitch-enum
CFLAGS += -fPIC
CFLAGS += -pedantic
CFLAGS += -std=c99

LIB_OBJS += src/fields.o
LIB_OBJS += src/fields_posix.o
LIB_NAME := libfields

SHARED_SUFFIX := so
STATIC_SUFFIX := a

ifeq ($(uname_S),Darwin)
	SHARED_SUFFIX := dylib
endif

SHARED_LIB := $(LIB_NAME).$(SHARED_SUFFIX)
STATIC_LIB := $(LIB_NAME).$(STATIC_SUFFIX)

VERSION_MAJOR := 0
VERSION_MINOR := 6.0

SHARED_LIB_MAJOR := $(LIB_NAME).$(SHARED_SUFFIX).$(VERSION_MAJOR)
SHARED_LIB_MINOR := $(SHARED_LIB_MAJOR).$(VERSION_MINOR)

ifeq ($(uname_S),Darwin)
	SHARED_LIB_MAJOR := $(LIB_NAME).$(VERSION_MAJOR).$(SHARED_SUFFIX)
	SHARED_LIB_MINOR := $(LIB_NAME).$(VERSION_MAJOR).$(VERSION_MINOR).$(SHARED_SUFFIX)
endif

SONAME := -soname,$(SHARED_LIB_MAJOR)

ifeq ($(uname_S),Darwin)
	SONAME := -install_name,$(SHARED_LIB_MAJOR)
endif

OBJS += examples/yahoo-finance.o
PROG := examples/yahoo-finance

V =
ifeq ($(strip $(V)),)
	E := @echo
	Q := @
else
	E := @\#
	Q :=
endif

all: examples $(SHARED_LIB) $(STATIC_LIB)
.PHONY: all

clean:
	$(E) "  CLEAN    "
	$(Q) $(RM) $(LIB_OBJS) $(OBJS) $(PROG) $(SHARED_LIB) $(STATIC_LIB)
	$(Q) $(MAKE) -C python clean
.PHONY: clean

examples: $(PROG)
.PHONY: examples

install: $(SHARED_LIB) $(STATIC_LIB)
	$(E) "  INSTALL  "
	$(Q) mkdir -p $(PREFIX)/include $(PREFIX)/lib
	$(Q) cp include/fields.h include/fields_posix.h $(PREFIX)/include
	$(Q) cp $(STATIC_LIB) $(PREFIX)/lib
.PHONY: install

test: $(SHARED_LIB)
	$(E) "  TEST     "
	$(Q) $(MAKE) -C python test
.PHONY: test

$(SHARED_LIB): $(LIB_OBJS)
	$(E) "  LINK     " $@
	$(Q) $(CC) $(LDFLAGS) -shared -Wl,$(SONAME) -o $@ $^

$(STATIC_LIB): $(LIB_OBJS)
	$(E) "  ARCHIVE  " $@
	$(Q) $(AR) rcs $@ $^

$(PROG): $(OBJS) $(STATIC_LIB)
	$(E) "  LINK     " $@
	$(Q) $(LD) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(E) "  COMPILE  " $@
	$(Q) $(CC) $(CFLAGS) -c -o $@ $<
