CC = gcc

CFLAGS = -Wall -Wextra -pedantic -std=c11

TARGET = hello
SRCS = hello.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)
	
%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

run: all
	./$(TARGET)
	@echo "Execution finished."

.PHONY: all clean run

