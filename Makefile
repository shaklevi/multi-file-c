CC = gcc
CFLAGS = -Wall -Wextra -g

BUILD_DIR = artifacts
TARGET = $(BUILD_DIR)/prog
OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/fibonacci.o

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

$(BUILD_DIR)/main.o: main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c main.c -o $(BUILD_DIR)/main.o

$(BUILD_DIR)/fibonacci.o: fibonacci.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c fibonacci.c -o $(BUILD_DIR)/fibonacci.o

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

