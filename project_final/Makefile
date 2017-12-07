.SUFFIXES: .c .o

define \n

endef

CC=gcc

SRCDIR=src/
TESTDIR=test/
INC=include/
LIBS=lib/

# SRCS:=$(wildcard src/*.c)
# OBJS:=$(SRCS:.c=.o)

# main source file
TARGET_SRC:=$(SRCDIR)main.c
TARGET_OBJ:=$(SRCDIR)main.o

# Include more files if you write another source file.
SRCS_FOR_LIB:=$(SRCDIR)page.c $(SRCDIR)bpt.c $(SRCDIR)hash.c $(SRCDIR)lru.c $(SRCDIR)algo.c \
	$(SRCDIR)transaction.c
OBJS_FOR_LIB:=$(SRCS_FOR_LIB:.c=.o)

SRCS_FOR_TEST:=$(TESTDIR)test_page.c $(TESTDIR)test_insert.c $(TESTDIR)tests.c $(TESTDIR)test_delete.c $(TESTDIR)test_table.c \
	$(TESTDIR)test_perf.c

CFLAGS := -g -fPIC -I $(INC) -O2

TARGET=main

all: $(TARGET)

$(TARGET): $(TARGET_OBJ)
	$(foreach SRC, $(SRCS_FOR_LIB), $(CC) $(CFLAGS) -o $(SRC:.c=.o) -c $(SRC) ; )
	make static_library
	$(CC) $(CFLAGS) -o $@ $^ -L $(LIBS) -lbpt

clean:
	# rm $(TARGET) $(TARGET_OBJ) $(OBJS_FOR_LIB) $(LIBS)*
	-rm -f $(TARGET_OBJ) $(OBJS_FOR_LIB) $(LIBS)* db/*.db desc main printall

library:
	gcc -shared -Wl,-soname,libbpt.so -o $(LIBS)libbpt.so $(OBJS_FOR_LIB)

static_library:
	ar cr $(LIBS)libbpt.a $(OBJS_FOR_LIB)

build_test: test/test_page.c
	$(foreach SRC, $(SRCS_FOR_TEST), $(CC) $(CFLAGS) -o $(SRC:.c=.o) -c $(SRC) ; )
	$(CC) $(CFLAGS) -o $(TESTDIR)tests $(SRCS_FOR_TEST) -L $(LIBS) -lbpt

describe: $(TESTDIR)describe.c
	$(CC) $(CFLAGS) -o desc $(TESTDIR)describe.c -L $(LIBS) -lbpt

printall: $(TESTDIR)printall.c
	$(CC) $(CFLAGS) -o printall $(TESTDIR)printall.c -L $(LIBS) -lbpt

at:
	make clean
	make main
	make build_test
	make describe
	test/tests

test_perf:
	test/tests perf 16
	test/tests perf 32
	test/tests perf 64
	test/tests perf 128
	test/tests perf 256
	test/tests perf 512
	test/tests perf 1024
	test/tests perf 2048
	test/tests perf 4096
	test/tests perf 8192
	test/tests perf 16384
	
