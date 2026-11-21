# oracle/oracle.mk -- oracle-side build and test targets, to be included from the team's
# top-level Makefile:        include oracle/oracle.mk
# and hooked into the existing target:        test: oracle-test oracle-real
# Run on its own with:       make -f oracle/oracle.mk oracle-test oracle-real
#
#   oracle-test : unit tests + differential test against the STAND-IN (checks the harness itself)
#   oracle-real : differential test (clusters tracked, NOT tracked, and a deeper --maxL 40 pass)
#                 and sweep test against the REAL structures
#
# Include path: include/unionfind.hpp is used. Outside the team repo (no include/ folder) the
# copy in scaffold/include/ is used instead; with neither, make stops with an error.
ORACLE_CXX    ?= g++
ORACLE_FLAGS  ?= -O2 -std=c++17 -Wall -Wextra
ORACLE_UF_INC := $(if $(wildcard include/unionfind.hpp),-Iinclude,$(if $(wildcard scaffold/include/unionfind.hpp),-Iscaffold/include,$(error include/unionfind.hpp not found)))
ORACLE_INC    ?= -Ioracle -Itests $(ORACLE_UF_INC)
ORACLE_BUILD  ?= build/oracle
ORACLE_SEED   ?= 171226
ORACLE_OPS    ?= 1000000
ORACLE_DEEP_OPS ?= 200000
ORACLE_SWEEPS ?= 2000
ORACLE_CC      = $(ORACLE_CXX) $(ORACLE_FLAGS) $(ORACLE_INC)

# Sources of the real structures: factory, Structure A, Structure B, sweep order helpers.
# Override REAL_SRC if the team's layout differs.
REAL_SRC ?= $(filter-out $(wildcard src/structB/test_*.cpp), \
             $(wildcard src/*.cpp src/structA/*.cpp src/structB/*.cpp))
REAL_INC ?= -Ioracle -Itests -Iinclude -Isrc

.PHONY: oracle-test oracle-unit oracle-diff oracle-real oracle-diff-real oracle-sweep-real

oracle-unit:
	@mkdir -p $(ORACLE_BUILD)
	$(ORACLE_CC) oracle/flood_fill.cpp tests/test_oracle.cpp    -o $(ORACLE_BUILD)/test_oracle
	$(ORACLE_CC) tests/test_checkers.cpp                        -o $(ORACLE_BUILD)/test_checkers
	$(ORACLE_CC) oracle/flood_fill.cpp tests/test_pcheckers.cpp -o $(ORACLE_BUILD)/test_pcheckers
	$(ORACLE_CC) oracle/flood_fill.cpp tests/test_harness.cpp   -o $(ORACLE_BUILD)/test_harness
	$(ORACLE_BUILD)/test_oracle
	$(ORACLE_BUILD)/test_checkers
	$(ORACLE_BUILD)/test_pcheckers
	$(ORACLE_BUILD)/test_harness

oracle-diff:
	@mkdir -p $(ORACLE_BUILD)
	$(ORACLE_CC) oracle/flood_fill.cpp oracle/diff_main.cpp -o $(ORACLE_BUILD)/diff_test
	$(ORACLE_BUILD)/diff_test --ops $(ORACLE_OPS) --seed $(ORACLE_SEED)
	$(ORACLE_BUILD)/diff_test --ops $(ORACLE_OPS) --seed $(ORACLE_SEED) --no-clusters

oracle-test: oracle-unit oracle-diff

oracle-diff-real:
	@test -n "$(REAL_SRC)" || (echo "no sources found: set REAL_SRC" && exit 1)
	@mkdir -p $(ORACLE_BUILD)
	$(ORACLE_CXX) $(ORACLE_FLAGS) -DUSE_REAL $(REAL_INC) \
	    oracle/flood_fill.cpp oracle/diff_main.cpp $(REAL_SRC) -o $(ORACLE_BUILD)/diff_test_real
	$(ORACLE_BUILD)/diff_test_real --ops $(ORACLE_OPS) --seed $(ORACLE_SEED)
	$(ORACLE_BUILD)/diff_test_real --ops $(ORACLE_OPS) --seed $(ORACLE_SEED) --no-clusters
	$(ORACLE_BUILD)/diff_test_real --ops $(ORACLE_DEEP_OPS) --seed $(ORACLE_SEED) --maxL 40 --check-every 10

oracle-sweep-real:
	@test -n "$(REAL_SRC)" || (echo "no sources found: set REAL_SRC" && exit 1)
	@mkdir -p $(ORACLE_BUILD)
	$(ORACLE_CXX) $(ORACLE_FLAGS) $(REAL_INC) \
	    oracle/flood_fill.cpp oracle/sweep_main.cpp $(REAL_SRC) -o $(ORACLE_BUILD)/sweep_test
	$(ORACLE_BUILD)/sweep_test --sweeps $(ORACLE_SWEEPS) --seed $(ORACLE_SEED)

oracle-real: oracle-diff-real oracle-sweep-real
