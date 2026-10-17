CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Werror -O2
CPPFLAGS ?=
LDLIBS ?= -pthread

BUILD_DIR = build
ENGINE = $(BUILD_DIR)/flashlock-engine
DEADLOCK_DEMO = $(BUILD_DIR)/deadlock-demo

.PHONY: all run test deadlock-demo clean

all: $(ENGINE)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(ENGINE): main.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) main.c -o $(ENGINE) $(LDLIBS)

$(DEADLOCK_DEMO): deadlock_demo.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) deadlock_demo.c -o $(DEADLOCK_DEMO) $(LDLIBS)

run: $(ENGINE)
	./$(ENGINE)

test:
	CC="$(CC)" sh ./tests/test_engine.sh

deadlock-demo: $(DEADLOCK_DEMO)
	./$(DEADLOCK_DEMO)

# Remove only the two outputs produced by this Makefile.
clean:
	rm -f $(ENGINE) $(DEADLOCK_DEMO)
	rmdir $(BUILD_DIR) 2>/dev/null || true
