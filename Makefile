CC ?= gcc

PYTHON ?= python

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

OBJS += src/fields.o
OBJS += src/fields_posix.o

LIB := libfields.so

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
	$(Q) $(RM) $(OBJS) $(LIB)
	$(Q) find . -name *.pyc | xargs $(RM)
.PHONY: clean

test: $(LIB)
	$(E) "  TEST     "
	$(Q) cd test; LD_LIBRARY_PATH=.. $(PYTHON) test_fields.py
.PHONY: test

$(LIB): $(OBJS)
	$(E) "  LINK     " $@
	$(Q) $(CC) $(CFLAGS) -shared -o $@ $^

%.o: %.c
	$(E) "  COMPILE  " $@
	$(Q) $(CC) $(CFLAGS) -c -o $@ $<
