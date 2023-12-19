FLAGS = -O0 -Wall -nostartfiles -ffreestanding -Iinclude -mgeneral-regs-only -g -fPIC
COPS = -w -nostdlib -nostartfiles -ffreestanding -Iinclude -mgeneral-regs-only
ASMOPS = -Iinclude 
ARMGNU ?= aarch64-linux-gnu
BUILD_DIR = build
SRC_DIR = src
USER_PROG = mailbox_test
INC = include

all : kernel8.img $(USER_PROG).img

clean:
	rm -f *.elf *.img *.o *.S *.s user/*.elf user/*.img 

$(BUILD_DIR)/%_c.o: $(SRC_DIR)/%.c
	mkdir -p $(@D)
	$(ARMGNU)-gcc $(COPS) -MMD -c $< -o $@

$(BUILD_DIR)/%_s.o: $(SRC_DIR)/%.S
	$(ARMGNU)-gcc $(ASMOPS) -MMD -c $< -o $@

C_FILES = $(wildcard $(SRC_DIR)/*.c)
ASM_FILES = $(wildcard $(SRC_DIR)/*.S)
OBJ_FILES = $(C_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%_c.o)
OBJ_FILES += $(ASM_FILES:$(SRC_DIR)/%.S=$(BUILD_DIR)/%_s.o)

DEP_FILES = $(OBJ_FILES:%.o=%.d)
-include $(DEP_FILES)

kernel8.img: kernel/*.S kernel/*.c $(SRC_DIR)/*.S $(SRC_DIR)/*.c 
	$(ARMGNU)-gcc $(FLAGS) -Tkernel/linker.ld $^ -o kernel8.elf -I$(INC)
	$(ARMGNU)-objcopy -O binary kernel8.elf kernel8.img

$(USER_PROG).img: user/$(USER_PROG).c $(SRC_DIR)/*.c $(SRC_DIR)/*.S
	$(ARMGNU)-gcc $(FLAGS) -Tuser/linker.ld $^ -o user/$(USER_PROG).elf
	$(ARMGNU)-objcopy -O binary user/$(USER_PROG).elf user/$(USER_PROG).img

py:
	sudo python3 load.py
cpio: 
	cd archive/rootfs && find . | cpio -o -H newc > ../../initramfs.cpio

screen:
	sudo screen /dev/ttyUSB0 115200
qemu:
	qemu-system-aarch64 -M raspi3 -kernel kernel8.img -serial null -serial stdio -initrd initramfs.cpio