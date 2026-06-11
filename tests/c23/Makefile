CC ?= gcc
PROFILE ?=
PYTHON ?= python3
RUN_MODE ?= auto

.PHONY: generate matrix matrix-sdcc-z80 clean

generate:
	$(PYTHON) tests/scripts/generate_suite.py

matrix: generate
	$(PYTHON) tests/scripts/run_matrix.py \
		$(if $(PROFILE),--profile $(PROFILE),--compiler $(CC)) \
		--run-mode $(RUN_MODE)

matrix-sdcc-z80: generate
	$(PYTHON) tests/scripts/run_matrix.py \
		--profile setups/sdcc-z80/profile-ucsim.json \
		--run-mode $(RUN_MODE)

clean:
	rm -rf build/results tests/cases docs/dist/c23-compatibility-matrix.md \
		docs/research/c23-feature-catalog.md
