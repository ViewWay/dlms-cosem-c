CC ?= gcc
CFLAGS ?= -Wall -Wextra -Wpedantic -std=c99 -Iinclude
AR ?= ar

SRCDIR = src
INCDIR = include
TESTDIR = tests
EXDIR = examples
BUILDDIR = build

SRC = $(wildcard $(SRCDIR)/*.c)
OBJ = $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SRC))
TEST_SRC = $(filter-out $(TESTDIR)/test_main.c,$(wildcard $(TESTDIR)/test_*.c))

.PHONY: all clean test example

all: $(BUILDDIR)/libdlms-cosem.a

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR)/libdlms-cosem.a: $(OBJ)
	$(AR) rcs $@ $^

test: $(BUILDDIR)/libdlms-cosem.a
	$(CC) $(CFLAGS) -o $(BUILDDIR)/test_runner $(TESTDIR)/test_main.c $(TEST_SRC) -L$(BUILDDIR) -ldlms-cosem -lm
	./$(BUILDDIR)/test_runner

example: $(BUILDDIR)/libdlms-cosem.a
	$(CC) $(CFLAGS) -o $(BUILDDIR)/meter_simulation $(EXDIR)/meter_simulation.c -L$(BUILDDIR) -ldlms-cosem

clean:
	rm -rf $(BUILDDIR)
