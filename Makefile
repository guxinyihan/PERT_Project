CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2
CPPFLAGS = -Iinclude
LDLIBS = -lm
CORE = src/model.c src/input.c src/graph.c src/pert.c src/probability.c src/output.c

all: pert.exe

pert.exe: src/main.c $(CORE) $(wildcard include/*.h)
	$(CC) $(CFLAGS) $(CPPFLAGS) src/main.c $(CORE) -o $@ $(LDLIBS)

tests/test_core.exe: tests/test_core.c $(CORE) $(wildcard include/*.h)
	$(CC) $(CFLAGS) $(CPPFLAGS) tests/test_core.c $(CORE) -o $@ $(LDLIBS)

test: pert.exe tests/test_core.exe
	./tests/test_core.exe

.PHONY: all test
