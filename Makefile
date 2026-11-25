# Makefile for FreeRTOS Multi-Core Criticality-Urgency Quadrant Demo (Linux POSIX Port)

PROJECT_SMP = freertos_demo_smp

BUILD_DIR = build
FREERTOS_DIR = FreeRTOS-Kernel
FREERTOS_TAG = V11.1.0
FREERTOS_PORT = $(FREERTOS_DIR)/portable/ThirdParty/GCC/Posix
FREERTOS_MEMMANG = $(FREERTOS_DIR)/portable/MemMang

KERNEL_OBJS = \
	$(BUILD_DIR)/tasks.o \
	$(BUILD_DIR)/queue.o \
	$(BUILD_DIR)/list.o \
	$(BUILD_DIR)/timers.o \
	$(BUILD_DIR)/event_groups.o \
	$(BUILD_DIR)/stream_buffer.o \
	$(BUILD_DIR)/port.o \
	$(BUILD_DIR)/wait_for_event.o \
	$(BUILD_DIR)/heap_4.o

INCLUDES = \
	-I./src \
	-I$(FREERTOS_DIR)/include \
	-I$(FREERTOS_PORT) \
	-I$(FREERTOS_PORT)/utils

CC = gcc
CFLAGS = -Wall -Wextra -O0 -g3 -pthread -DprojCOVERAGE_TEST=0
LDFLAGS = -pthread -lrt -lm

all: smp

smp: $(BUILD_DIR)/$(PROJECT_SMP)

smp-8core: CFLAGS += -DNUM_CORES=8
smp-8core: $(BUILD_DIR)/$(PROJECT_SMP)

fetch-freertos:
	@if [ ! -d "$(FREERTOS_DIR)" ]; then \
		git clone --depth 1 --branch $(FREERTOS_TAG) https://github.com/FreeRTOS/FreeRTOS-Kernel.git $(FREERTOS_DIR); \
	else \
		echo "$(FREERTOS_DIR) already exists."; \
	fi

check-freertos:
	@if [ ! -f "$(FREERTOS_DIR)/tasks.c" ]; then \
		echo "Error: $(FREERTOS_DIR) not found. Run 'make fetch-freertos' (or 'git submodule update --init')."; \
		exit 1; \
	fi

$(BUILD_DIR)/$(PROJECT_SMP): check-freertos src/freertos_quadrant_demo_smp.c src/FreeRTOSConfig_linux.h $(KERNEL_OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) src/freertos_quadrant_demo_smp.c $(KERNEL_OBJS) $(LDFLAGS) -o $@
	@echo "Multi-core SMP build complete: $@"

$(BUILD_DIR)/tasks.o: $(FREERTOS_DIR)/tasks.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/queue.o: $(FREERTOS_DIR)/queue.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/list.o: $(FREERTOS_DIR)/list.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/timers.o: $(FREERTOS_DIR)/timers.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/event_groups.o: $(FREERTOS_DIR)/event_groups.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/stream_buffer.o: $(FREERTOS_DIR)/stream_buffer.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/port.o: $(FREERTOS_PORT)/port.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/wait_for_event.o: $(FREERTOS_PORT)/utils/wait_for_event.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/heap_4.o: $(FREERTOS_MEMMANG)/heap_4.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

run: run-smp

run-smp: $(BUILD_DIR)/$(PROJECT_SMP)
	./$(BUILD_DIR)/$(PROJECT_SMP)

clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "Targets: fetch-freertos | all (smp) | smp-8core | run-smp | clean"

.PHONY: all smp smp-8core fetch-freertos check-freertos run run-smp clean help
