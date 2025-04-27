CC = gcc

CFLAGS = -std=c11 -g -Wall -Wextra -pedantic -D_POSIX_C_SOURCE=199309L

LDFLAGS = -pthread

TARGET = server

SRCDIR  = src
BUILDDIR = build
DEBUGDIR = $(BUILDDIR)/debug
RELEASEDIR = $(BUILDDIR)/release

OUT_DIR = $(DEBUGDIR)

SOURCES = $(wildcard $(SRCDIR)/*.c) # Автоматически найти все .c в src
#SOURCES = $(SRCDIR)/server.c $(SRCDIR)/telemetry.c $(SRCDIR)/conf.c # Или перечислить явно

OBJECTS = $(patsubst $(SRCDIR)/%.c, $(OUT_DIR)/%.o, $(SOURCES))

HEADERS = $(wildcard $(SRCDIR)/*.h)



ifeq ($(MODE), release)
  INFO_MSG = "Release mode"
  CFLAGS = -std=c11 -O2 -Wall -Wextra -pedantic 
  OUT_DIR = $(RELEASEDIR)
  OBJECTS = $(patsubst $(SRCDIR)/%.c, $(OUT_DIR)/%.o, $(SOURCES)) 
else
  INFO_MSG = "Debug mode (default)"
endif

PROG = $(OUT_DIR)/$(TARGET)

all: $(PROG)
	@echo $(INFO_MSG) : Build finished for $(PROG)

$(PROG): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	@mkdir -p $(OUT_DIR) # Убедимся, что директория существует
	$(CC) $(CFLAGS) $(OBJECTS) -o $@ $(LDFLAGS) # Используем CFLAGS и LDFLAGS

$(OUT_DIR)/%.o: $(SRCDIR)/%.c $(HEADERS) Makefile
	@echo "Compiling $< -> $@"
	@mkdir -p $(OUT_DIR) # Убедимся, что директория существует
	$(CC) -c $(CFLAGS) $< -o $@ # $< - имя зависимости (.c), $@ - имя цели (.o)

clean:
	@echo "Cleaning build directories..."
	@rm -rf $(BUILDDIR)/* $(TARGET) # Удаляем всю директорию build и исполняемый файл в корне (если есть)

run: all
	@echo "Running $(PROG)..."
	./$(PROG)

valgrind:
ifeq ($(MODE), release)
	@echo "Valgrind should be run in debug mode. Use 'make valgrind' (without MODE=release)."
else
	@echo "Running $(PROG) with Valgrind..."
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(PROG)
endif

start: run

.PHONY: all clean run valgrind start