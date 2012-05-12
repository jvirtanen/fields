CC ?= gcc
LD := $(CC)

PYTHON ?= python

CFLAGS += -Isrc
CFLAGS += -O3
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Wshadow
CFLAGS += -Wswitch-default
CFLAGS += -Wswitch-enum
CFLAGS += -pedantic
CFLAGS += -std=c99

OBJS += src/sheets.o
OBJS += test/dump.o
PROG := test/dump

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
	$(Q) rm -f $(OBJS) $(PROG)
	$(Q) find . -name *.pyc | xargs rm -f
.PHONY: clean

test: $(PROG)
	$(E) "  TEST     "
	$(Q) cd test; $(PYTHON) test.py
.PHONY: test

$(PROG): $(OBJS)
	$(E) "  LINK     " $@
	$(Q) $(LD) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(E) "  COMPILE  " $@
	$(Q) $(CC) $(CFLAGS) -c -o $@ $<
