# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0

all:

CXXFLAGS ?= -g -Og -Wdeprecated -Werror=format-security -Wredundant-decls -Wpointer-arith -Wmissing-declarations -Werror=return-type -Wno-tautological-compare -Wno-constant-logical-operand -Woverloaded-virtual -Wsign-promo
CCACHE   ?= $(if $(CCACHE_DIR), ccache)

ifeq (default,$(origin CXX))
  ifneq (,$(shell which clang++))
    CXX	:= clang++
    CC  := clang
  endif
endif

%.o: %.cc
	$(CCACHE) $(CXX) -std=gnu++17 -Wall $(CXXFLAGS) -c $< -o $@

hello: hello.o
	$(CCACHE) $(CXX) $^ -o $@ -lboost_system -lboost_filesystem
hello.o: ../src/webhead.cc ../src/webhead.hh

clean:
	rm -f hello *.o */*.o

all: hello
