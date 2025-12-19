CC = cc
CFLAGS = -g -O3 -Iinclude -Wall -Wpedantic
LDFLAGS = -lm -lraylib -lraygui
SRC_DIR = src
BUILD_DIR = build
TARGET = $(BUILD_DIR)/editor

SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(subst $(SRC_DIR), $(BUILD_DIR), $(patsubst %.c,%.o,$(SRC)))

run: $(TARGET)
	./$(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@if [ ! -d "$(BUILD_DIR)" ]; then mkdir -p $(BUILD_DIR); fi
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -fr $(OBJ) $(TARGET)
	rm -fr $(BUILD_DIR)
