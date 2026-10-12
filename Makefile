.DEFAULT_GOAL := run

DBG = DEBUG

TARGET  = matrix
BUILD_DIR = build
SRCS    = src/main.cpp test/test_matrix.cpp
OBJS    = $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SRCS))

CC = g++
DEBUG_FLAGS = -D _DEBUG \
				-ggdb3 \
				-fsanitize=address,alignment,bool,bounds,enum,float-cast-overflow,float-divide-by-zero,integer-divide-by-zero,leak,nonnull-attribute,null,object-size,return,returns-nonnull-attribute,shift,signed-integer-overflow,undefined,unreachable,vla-bound,vptr

CFLAGS = -std=c++17 -Wall \
		-Wextra \
		-Weffc++ \
		-Waggressive-loop-optimizations \
		-Wc++14-compat -Wmissing-declarations\
		-Wcast-align \
		-Wcast-qual \
		-Wchar-subscripts \
		-Wconditionally-supported \
		-Wconversion \
		-Wctor-dtor-privacy \
		-Wempty-body \
		-Wfloat-equal\
		-Wformat-nonliteral \
		-Wformat-security \
		-Wformat-signedness \
		-Wformat=2 \
		-Winline \
		-Wlogical-op \
		-Wnon-virtual-dtor \
		-Wopenmp-simd \
		-Woverloaded-virtual \
		-Wpacked \
		-Wpointer-arith \
		-Winit-self \
		-Wredundant-decls \
		-Wshadow \
		-Wsign-conversion \
		-Wsign-promo \
		-Wstrict-null-sentinel \
		-Wstrict-overflow=2 \
		-Wsuggest-attribute=noreturn \
		-Wsuggest-final-methods \
		-Wsuggest-final-types \
		-Wsuggest-override \
		-Wswitch-default \
		-Wswitch-enum \
		-Wsync-nand \
		-Wundef \
		-Wunreachable-code \
		-Wunused \
		-Wuseless-cast \
		-Wvariadic-macros \
		-Wno-literal-suffix \
		-Wno-missing-field-initializers \
		-Wno-narrowing \
		-Wno-old-style-cast \
		-Wno-varargs \
		-Wstack-protector \
		-fcheck-new -fsized-deallocation \
		-fstack-protector \
		-fstrict-overflow \
		-flto-odr-type-merging \
		-fno-omit-frame-pointer \
		-Wlarger-than=30000 \
		-Wstack-usage=8192 \
		-pie \
		-fPIE \
		-Werror=vla \
		-Iinclude
		
RELEASE_FLAGS = -O2 -march=native -g -D NDEBUG -flto

ifeq ($(DBG), DEBUG)
    CFLAGS += $(DEBUG_FLAGS)
else
    CFLAGS += $(RELEASE_FL.DEFAULT_GOAL := run

DBG = DEBUG

TARGET  = cache
BUILD_DIR = build
SRCS    = src/main.cpp src/config.cpp tests/unit/unit_cache_test.cpp tests/test_runner.cpp
OBJS    = $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SRCS))

CC = g++
DEBUG_FLAGS = -D _DEBUG \
				-ggdb3 \
				-fsanitize=address,alignment,bool,bounds,enum,float-cast-overflow,float-divide-by-zero,integer-divide-by-zero,leak,nonnull-attribute,null,object-size,return,returns-nonnull-attribute,shift,signed-integer-overflow,undefined,unreachable,vla-bound,vptr

CFLAGS = -std=c++17 -Wall \
		-Wextra \
		-Weffc++ \
		-Waggressive-loop-optimizations \
		-Wc++14-compat -Wmissing-declarations\
		-Wcast-align \
		-Wcast-qual \
		-Wchar-subscripts \
		-Wconditionally-supported \
		-Wconversion \
		-Wctor-dtor-privacy \
		-Wempty-body \
		-Wfloat-equal\
		-Wformat-nonliteral \
		-Wformat-security \
		-Wformat-signedness \
		-Wformat=2 \
		-Winline \
		-Wlogical-op \
		-Wnon-virtual-dtor \
		-Wopenmp-simd \
		-Woverloaded-virtual \
		-Wpacked \
		-Wpointer-arith \
		-Winit-self \
		-Wredundant-decls \
		-Wshadow \
		-Wsign-conversion \
		-Wsign-promo \
		-Wstrict-null-sentinel \
		-Wstrict-overflow=2 \
		-Wsuggest-attribute=noreturn \
		-Wsuggest-final-methods \
		-Wsuggest-final-types \
		-Wsuggest-override \
		-Wswitch-default \
		-Wswitch-enum \
		-Wsync-nand \
		-Wundef \
		-Wunreachable-code \
		-Wunused \
		-Wuseless-cast \
		-Wvariadic-macros \
		-Wno-literal-suffix \
		-Wno-missing-field-initializers \
		-Wno-narrowing \
		-Wno-old-style-cast \
		-Wno-varargs \
		-Wstack-protector \
		-fcheck-new -fsized-deallocation \
		-fstack-protector \
		-fstrict-overflow \
		-flto-odr-type-merging \
		-fno-omit-frame-pointer \
		-Wlarger-than=30000 \
		-Wstack-usage=8192 \
		-pie \
		-fPIE \
		-Werror=vla \
		-Iinclude
		
RELEASE_FLAGS = -O2 -march=native -g -D NDEBUG -flto

ifeq ($(DBG), DEBUG)
    CFLAGS += $(DEBUG_FLAGS)
else
    CFLAGS += $(RELEASE_FLAGS)
endif


all: $(TARGET)

$(TARGET): $(OBJS)
	@$(CC) $(CFLAGS) -o $@ $(OBJS) -lgtest -lgtest_main -pthread

$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

run: all
	@./$(TARGET)

clean:
	@rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all run cleanAGS)
endif


all: $(TARGET)

$(TARGET): $(OBJS)
	@$(CC) $(CFLAGS) -o $@ $(OBJS) -lgtest -lgtest_main -pthread

$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

run: all
	@./$(TARGET) config.cfg

clean:
	@rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all run clean