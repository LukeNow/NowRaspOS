.DEFAULT_GOAL := compile

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
CFLAGS = -Wall -O2 -Wextra -ffreestanding -mcpu=cortex-a53 -march=armv8-a -mgeneral-regs-only
LFLAGS = -ffreestanding -O2 -nostdlib
QEMU_FLAGS = -serial null -serial stdio -smp 4 -dtb bcm2710-rpi-3-b-plus.dtb
QEMU_DBG_FLAGS = -S -s

QEMU = qemu-system-aarch64
CC = aarch64-elf-gcc
LD = aarch64-elf-ld
OBJCOPY = aarch64-elf-objcopy
GDB = aarch64-elf-gdb

SRC_KERNEL = src/kernel
SRC_COMMON = src/common
HEADER_INCLUDE = include

KERNSOURCES = $(wildcard $(SRC_KERNEL)/*.c)
COMMONSOURCES = $(wildcard $(SRC_COMMON)/*.c)
KERNSOURCESASM = $(wildcard $(SRC_KERNEL)/*.S)
COMMONSOURCESASM = $(wildcard $(SRC_COMMON)/*.S)

BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)

OBJECTS = $(patsubst $(SRC_KERNEL)/%.c, $(OBJ_DIR)/%.o, $(KERNSOURCES))
OBJECTS += $(patsubst $(SRC_COMMON)/%.c, $(OBJ_DIR)/%.o, $(COMMONSOURCES))
OBJECTS += $(patsubst $(SRC_KERNEL)/%.S, $(OBJ_DIR)/%.o, $(KERNSOURCESASM))
OBJECTS += $(patsubst $(SRC_COMMON)/%.S, $(OBJ_DIR)/%.o, $(COMMONSOURCESASM))
HEADERS = $(wildcard $(HEADER_INCLUDE)/*.h)

IMG_NAME = NowRaspOS
ELF = $(BUILD_DIR)/$(IMG_NAME).elf
IMG = $(BUILD_DIR)/$(IMG_NAME).img

build: $(OBJECTS) $(HEADERS)
	$(CC) -T linker.ld -o $(BUILD_DIR)/$(IMG_NAME).elf $(LFLAGS) $(OBJECTS)
	$(OBJCOPY) $(ELF) -O binary $(IMG)

$(OBJ_DIR)/%.o: $(SRC_KERNEL)/%.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -I$(SRC_KERNEL) -I$(HEADER_INCLUDE) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_KERNEL)/%.S
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -I$(SRC_KERNEL) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_COMMON)/%.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -I$(SRC_KERNEL) -I$(HEADER_INCLUDE) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_COMMON)/%.S
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -I$(SRC_COMMON) -c $< -o $@

kernel8.img: start.o $(OBJS)
	aarch64-elf-ld -nostdlib start.o $(OBJS) -T link.ld -o kernel8.elf
	aarch64-elf-objcopy -O binary kernel8.elf kernel8.img

clean:
	rm -rf $(BUILD_DIR)

compile: clean build

run: compile
	qemu-system-aarch64 -M raspi3b -kernel $(IMG) $(QEMU_FLAGS)

debug:
	$(GDB) $(ELF)

debugrun: compile gdbinit
	qemu-system-aarch64 -M raspi3b -no-reboot $(QEMU_FLAGS) -kernel $(IMG) $(QEMU_DBG_FLAGS)

.PHONY: gdbinit
gdbinit:
	echo "target remote localhost:1234" > .gdbinit
	echo "break _start" >> .gdbinit
	echo "layout asm regs" >> .gdbinit
 