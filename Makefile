CC ?= gcc

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

LIB := $(LIB_NAME).so

V =
ifeq ($(strip $(V)),)
	E := @echo
	Q := @
else
	E := @\#
	Q :=
endif

all: test
.PHONY: all

clean:
	$(E) "  CLEAN    "
	$(Q) $(RM) $(LIB) $(LIB_OBJS)
	$(Q) cd python; $(MAKE) clean
.PHONY: clean

test: $(LIB)
	$(E) "  TEST     "
	$(Q) cd python; $(MAKE) test
.PHONY: test

$(LIB): $(LIB_OBJS)
	$(E) "  LINK     " $@
	$(Q) $(CC) $(CFLAGS) -shared -o $@ $^

%.o: %.c
	$(E) "  COMPILE  " $@
	$(Q) $(CC) $(CFLAGS) -c -o $@ $<
