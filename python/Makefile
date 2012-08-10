PYTHON ?= python

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
	$(Q) find . -name *.pyc | xargs $(RM)
.PHONY: clean

test:
	$(E) "  TEST     "
	$(Q) LD_LIBRARY_PATH=.. python test_fields.py
.PHONY: test
