BUILD_DIR ?= build
.PHONY: all test run clean
all:
	cmake -S . -B $(BUILD_DIR)
	cmake --build $(BUILD_DIR) --parallel
test: all
	ctest --test-dir $(BUILD_DIR) --output-on-failure
run: all
	./$(BUILD_DIR)/matrix_cli $(ARGS)
clean:
	cmake --build $(BUILD_DIR) --target clean
