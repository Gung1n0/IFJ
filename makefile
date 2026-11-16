# xbadint00
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -I$(SRC_DIR)

SRC_DIR = src
OBJ_DIR = obj
TARGET = main
ARCHIVE = IFJprojekt26.zip

SRCS = $(wildcard $(SRC_DIR)/*.c)
HDRS = $(wildcard $(SRC_DIR)/*.h)
OBJ = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# DEFAULT MAKE TARGETS

.PHONY: build all
build: $(TARGET)

all: build

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HDRS) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
# CHECK OF SOURCE FILES

.PHONY: check
check:
	@echo "Running clang-format..."
	clang-format --dry-run --Werror $(SRCS) $(HDRS)
	@echo "Running cppcheck..."
	cppcheck --enable=all --error-exitcode=1 --suppress=missingIncludeSystem -I$(SRC_DIR) $(SRCS)
	@echo "Running clang-tidy..."
	clang-tidy $(SRCS) -- $(CFLAGS)



# BUILD AND CHECK OF OBJECT/BIN FILES

.PHONY: build-check
build-check: check
	$(MAKE) clean
	$(MAKE) build CFLAGS="$(CFLAGS) -Werror"

$(OBJ_DIR):
	mkdir -p $@

.PHONY: format
format:
	clang-format -i $(SRCS) $(HDRS)

# CLEAN OF BUILD

.PHONY: clean
clean:
	@echo "Cleanup of build files . ..."

	rm -f $(OBJ_DIR)/*
	rm -rf $(OBJ_DIR)
	rm -f $(TARGET)
	rm -f $(ARCHIVE)
	rm -rf is_it_ok_test

.PHONY: zip
zip: clean
	@echo "Creating zip file ..."
	zip -r $(ARCHIVE) Makefile $(SRC_DIR) dokumentace.pdf rozdeleni rozsireni -x '*/.*'
	./is_it_ok.sh $(ARCHIVE) is_it_ok_test