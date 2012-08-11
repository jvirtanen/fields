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
	$(Q) $(RM) -r .tox/
	$(Q) $(RM) MANIFEST
.PHONY: clean

test:
	$(E) "  TEST     "
	$(Q) LD_LIBRARY_PATH=.. python test_fields.py
.PHONY: test
