# Convenience Makefile wrapper around CMake
.PHONY: all build clean run

BUILD_DIR ?= build

all: build

build:
	cmake -S . -B $(BUILD_DIR)
	cmake --build $(BUILD_DIR) -j

clean:
	rm -rf $(BUILD_DIR)

run: build
	$(BUILD_DIR)/src/cmini examples/hello.cmini -o out.ll
	@echo "Generated out.ll"
