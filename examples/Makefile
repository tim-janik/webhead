# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0

all:

CXXFLAGS ?= -g -Og

ifeq (default,$(origin CXX))
  ifneq (,$(shell which clang++))
    CXX	:= clang++
    CC  := clang
  endif
endif

%.o: %.cc
	$(CXX) -std=gnu++17 -Wall $(CXXFLAGS) -c $< -o $@

hello: hello.o
	$(CXX) $^ -o $@ -lboost_system -lboost_filesystem

clean:
	rm -f hello *.o */*.o

all: hello
