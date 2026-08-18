CC ?= gcc
PROFILE ?=
PYTHON ?= python3
RUN_MODE ?= auto
XCC ?=

.PHONY: generate matrix matrix-sdcc-z80 corpus test clean

generate:
	$(PYTHON) tests/scripts/generate_suite.py

matrix: generate
	$(PYTHON) tests/scripts/run_matrix.py \
		$(if $(PROFILE),--profile $(PROFILE),--compiler $(CC)) \
		$(if $(XCC),--compiler $(XCC)) \
		--run-mode $(RUN_MODE)

matrix-sdcc-z80: generate
	$(PYTHON) tests/scripts/run_matrix.py \
		--profile setups/sdcc-z80/profile-ucsim.json \
		--run-mode $(RUN_MODE)

corpus:
	XCC=$(if $(XCC),$(XCC),$(abspath ../../../../bin/x/bin/xcc)) bash corpus/run_all.sh

test:
	$(MAKE) matrix PROFILE=setups/xcc-z80/profile-xcc-z80.json RUN_MODE=auto
	$(MAKE) corpus

clean:
	rm -rf build/results tests/cases docs/dist/c23-compatibility-matrix.md \
		docs/research/c23-feature-catalog.md
