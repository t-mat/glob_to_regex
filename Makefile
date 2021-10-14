.PHONY: default all clean test-all test-0 0-glob-to-regex 1-glob-dirwalk 2-glob-rules

default: all

CXXFLAGS  ?= -O3 -std=c++17 -fPIE -g -I .

$(V)$(VERBOSE).SILENT:  # V=1 or VERBOSE=1 enables verbose mode.

all: test-all example-all

clean:
	$(RM) tests/*.o
	$(RM) test-0
	$(RM) examples/*.o
	$(RM) 0-glob-to-regex
	$(RM) 1-glob-dirwalk
	$(RM) 2-glob-rules

test-all: test-0
	echo test-0          && ./test-0

test-0: tests/test-0.o
	$(CXX) -o $@ $^

example-all: 0-glob-to-regex 1-glob-dirwalk 2-glob-rules
	echo 0-glob-to-regex && ./0-glob-to-regex
	echo 1-glob-dirwalk  && ./1-glob-dirwalk
	echo 2-glob-rules && ./2-glob-rules

0-glob-to-regex: examples/0-glob-to-regex.o
	$(CXX) -o $@ $^

1-glob-dirwalk: examples/1-glob-dirwalk.o
	$(CXX) -o $@ $^

2-glob-rules: examples/2-glob-rules.o
	$(CXX) -o $@ $^
