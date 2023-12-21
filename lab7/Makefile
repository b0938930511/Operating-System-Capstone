.PHONY: all clean qemu-kernel on-board

ARMGNU ?= aarch64-linux-gnu

FLAGS = -O0 -Wall -nostartfiles -ffreestanding -mgeneral-regs-only -g -fPIC
INC = include
LIB = lib
CPIO = archive/initramfs.cpio
USER_PROG = fs_test

all: kernel8.img $(CPIO) $(USER_PROG).img

kernel8.img: kernel/*.S kernel/*.c $(LIB)/*.S $(LIB)/*.c rd.o
	$(ARMGNU)-gcc $(FLAGS) -Tkernel/linker.ld $^ -o kernel8.elf -I$(INC)
	$(ARMGNU)-objcopy -O binary kernel8.elf kernel8.img

$(USER_PROG).img: user/$(USER_PROG).c user/$(LIB)/*.c
	$(ARMGNU)-gcc $(FLAGS) -Tuser/linker.ld $^ -o $(USER_PROG).elf -Iuser/$(INC)
	$(ARMGNU)-objcopy -O binary $(USER_PROG).elf $(USER_PROG).img

$(CPIO): $(USER_PROG).img
	cd archive/rootfs && find . | cpio -o -H newc > ../initramfs.cpio

rd.o:$(CPIO)
	aarch64-linux-gnu-ld -r -b binary -o rd.o $(CPIO)

qemu-kernel:
	qemu-system-aarch64 -M raspi3 -kernel kernel8.img -display none -serial null -serial stdio -initrd archive/initramfs.cpio

qemu-debug:
	aarch64-linux-gnu-objdump -d kernel8.elf > kernel8.S
	qemu-system-aarch64 -M raspi3 -kernel kernel8.img -display none -serial null -serial stdio -initrd archive/initramfs.cpio -d in_asm

# qemu-kernel:
# 	qemu-system-aarch64 -M raspi3 -kernel kernel8.img -display none -serial null -serial stdio -initrd $(CPIO)

# qemu-debug:
# 	aarch64-linux-gnu-objdump -d kernel8.elf > kernel8.S
# 	qemu-system-aarch64 -M raspi3 -kernel kernel8.img -display none -serial null -serial stdio -initrd $(CPIO) -d in_asm

on-board:
	sudo screen /dev/ttyUSB0 115200

clean:
	rm -f *.elf *.img *.cpio *.o *.S *.s