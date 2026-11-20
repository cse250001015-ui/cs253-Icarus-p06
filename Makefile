# Percolation (Project 6). Targets: all, test, app, bench, adversary, clean.
# Convention: any .cpp with a main() lives in tests/, app/, bench/ or adversary/.
#             src/ and oracle/ contain library code only.
# Tests use objects in build/dbg/ (asserts active). app and bench use objects in build/rel/
# built with -DNDEBUG so asserts do not distort timings.
CXX      ?= g++
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra
INC      := -Iinclude -Isrc -Ioracle -Itests -I.

LIB_SRC := $(filter-out %_main.cpp $(wildcard src/structB/test_*.cpp), \
           $(wildcard src/*.cpp) $(wildcard src/structA/*.cpp) $(wildcard src/structB/*.cpp) \
           $(wildcard oracle/*.cpp))
DBG_OBJ := $(LIB_SRC:%.cpp=build/dbg/%.o)
REL_OBJ := $(LIB_SRC:%.cpp=build/rel/%.o)

.PHONY: all test app bench adversary clean
all: app bench adversary

build/dbg/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INC) -MMD -MP -c $< -o $@

build/rel/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -DNDEBUG $(INC) -MMD -MP -c $< -o $@

build/percolation_app: $(wildcard app/*.cpp) $(REL_OBJ)
	$(CXX) $(CXXFLAGS) -DNDEBUG $(INC) $^ -o $@
BENCH_BIN := $(patsubst bench/%.cpp,build/%,$(wildcard bench/bench_*.cpp))
build/bench: bench/main.cpp $(REL_OBJ)
	$(CXX) $(CXXFLAGS) -DNDEBUG $(INC) $^ -o $@

build/bench_%: bench/bench_%.cpp $(REL_OBJ)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -DNDEBUG $(INC) $^ -o $@

bench: build/bench $(BENCH_BIN)
	./build/bench

# Each adversary/gen_*.cpp is its own program (its own main) and is built on its own.
ADV_BIN := $(patsubst adversary/%.cpp,build/%,$(wildcard adversary/gen_*.cpp))
build/gen_%: adversary/gen_%.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INC) $< -o $@

app: build/percolation_app
adversary: $(ADV_BIN)

# Tests are built WITHOUT -DNDEBUG so assert() is active.
test: $(DBG_OBJ)
	@mkdir -p build/tests
	@for f in tests/test_*.cpp; do \
	  [ -e "$$f" ] || continue; \
	  n=build/tests/$$(basename $$f .cpp); \
	  if [ "$$f" = tests/test_replay.cpp ]; then \
	    $(CXX) $(CXXFLAGS) $(INC) $$f app/replay.cpp $(DBG_OBJ) -o $$n || exit 1; \
	  else \
	    $(CXX) $(CXXFLAGS) $(INC) $$f $(DBG_OBJ) -o $$n || exit 1; \
	  fi; \
	  echo "== $$n"; $$n || exit 1; \
	done

clean:
	rm -rf build

-include $(DBG_OBJ:.o=.d) $(REL_OBJ:.o=.d)
