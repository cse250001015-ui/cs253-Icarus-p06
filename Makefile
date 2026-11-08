# Percolation (Project 6). Targets: all, test, app, bench, adversary, clean.
# Convention: any .cpp with a main() lives in tests/, app/, bench/ or adversary/.
#             src/ and oracle/ contain library code only.
CXX      ?= g++
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra
INC      := -Iinclude -Isrc -I.

LIB_SRC := $(wildcard src/*.cpp) $(wildcard src/structA/*.cpp) $(wildcard src/structB/*.cpp) \
           $(wildcard oracle/*.cpp)
LIB_OBJ := $(LIB_SRC:%.cpp=build/%.o)

.PHONY: all test app bench adversary clean
all: app bench adversary

build/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INC) -MMD -MP -c $< -o $@

build/percolation_app: $(wildcard app/*.cpp) $(LIB_OBJ)
	$(CXX) $(CXXFLAGS) $(INC) $^ -o $@
build/bench: $(wildcard bench/*.cpp) $(LIB_OBJ)
	$(CXX) $(CXXFLAGS) $(INC) $^ -o $@
# Each adversary/gen_*.cpp is its own program (its own main) and is built on its own.
ADV_BIN := $(patsubst adversary/%.cpp,build/%,$(wildcard adversary/gen_*.cpp))
build/gen_%: adversary/gen_%.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INC) $< -o $@

app: build/percolation_app
bench: build/bench
adversary: $(ADV_BIN)

# Tests are built WITHOUT -DNDEBUG so assert() is active.
test: $(LIB_OBJ)
	@mkdir -p build/tests
	@for f in tests/test_*.cpp; do \
	  n=build/tests/$$(basename $$f .cpp); \
	  $(CXX) $(CXXFLAGS) $(INC) $$f $(LIB_OBJ) -o $$n || exit 1; \
	  echo "== $$n"; $$n || exit 1; \
	done

clean:
	rm -rf build

-include $(LIB_OBJ:.o=.d)
