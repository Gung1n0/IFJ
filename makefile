CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -Isrc

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)


SRC_DIR = src
OBJ_DIR = obj

TARGET = main

# DEFAULT MAKE TARGETS
build: $(TARGET)

all: build


$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

# CREATION OF OBJECT FILES
check: $(OBJ)
	clang-tidy $(SRCS) -- $(CFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(wildcard $(SRC_DIR)/*.h) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# BUILD AND CHECK OF OBJECT/BIN FILES
build-check: check
	$(MAKE) build

$(OBJ_DIR):
	mkdir -p $@

format:
	clang-format -i $(SRCS) $(wildcard $(SRC_DIR)/*.h)

# CLEAN OF BUILD
clean:
	@echo "Cleanup of build files . ..."

	rm -f $(OBJ_DIR)/*
	rm -rf $(OBJ_DIR)

	rm -f $(TARGET)

.PHONY: build check build-check clean all format