#makefile
CC = gcc
CFLAGS = -std=c11 -g2 -ggdb -pedantic -W -Wall -Wextra

.PHONY: clean
.PHONY: valgrind
.PHONY: start

.SUFFIXES:
.SUFFIXES: .c .o

DEBUG   = ./build/debug
RELEASE = ./build/release
OUT_DIR = $(DEBUG)
vpath %.c src
vpath %.h src
vpath %.o build/debug

ifeq ($(MODE), release)
  CFLAGS = -std=c11 -pedantic -W -Wall -Wextra -Werror
  OUT_DIR = $(RELEASE)
  vpath %.o build/release
endif


objects =  $(OUT_DIR)/hello.o

prog = $(OUT_DIR)/hello

all: $(prog)

$(prog) : $(objects)
	$(CC) $(CFLAGS) $(objects) -o $@

$(OUT_DIR)/%.o : %.c
	$(CC) -c $(CFLAGS) $^ -o $@

clean:
	@rm -rf $(DEBUG)/* $(RELEASE)/* hello

run: all
	./$(OUT_DIR)/hello

valgrind:
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(OUT_DIR)/hello

start:
	./$(OUT_DIR)/hello
