.PHONY:all clean
LINKER:=linker.ld
CFLAGS:=-nostdinc -nostdlib -nostartfiles -no-stack-protector

all:
	make clean
	make user_prog.img
	cd rootfs&&find . | cpio -o -H newc > ../initramfs.cpio

user_prog.img:user_prog.o
	aarch64-linux-gnu-ld -T $(LINKER) -o tmp.elf $<
	aarch64-linux-gnu-objcopy -O binary tmp.elf $@
	cp $@ ./rootfs/$@
	rm $@
	rm *.o

user_prog.o:user_prog.s
	aarch64-linux-gnu-gcc -g -o $@ $(CFLAG) -c $<

clean:
	-rm *.cpio &rm rootfs/user_prog.img