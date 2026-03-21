AS = gcc
CC = gcc
LD = ld
CFLAGS = -ffreestanding -m32 -Wall -Wextra
LDFLAGS = -m elf_i386

all: freeze.iso

start.o: FreezeProject/start.S
	$(CC) $(CFLAGS) -c FreezeProject/start.S -o start.o

kernel.o: FreezeProject/kernel.c
	$(CC) $(CFLAGS) -c FreezeProject/kernel.c -o kernel.o

crt.o: FreezeProject/crt.c
	$(CC) $(CFLAGS) -c FreezeProject/crt.c -o crt.o

serial.o: FreezeProject/serial.c
	$(CC) $(CFLAGS) -c FreezeProject/serial.c -o serial.o
input.o: FreezeProject/input.c
	$(CC) $(CFLAGS) -c FreezeProject/input.c -o input.o
memory.o: FreezeProject/memory.c
	$(CC) $(CFLAGS) -c FreezeProject/memory.c -o memory.o
rtc.o: FreezeProject/rtc.c
	$(CC) $(CFLAGS) -c FreezeProject/rtc.c -o rtc.o
shell.o: FreezeProject/shell.c
	$(CC) $(CFLAGS) -c FreezeProject/shell.c -o shell.o
timer.o: FreezeProject/timer.c
	$(CC) $(CFLAGS) -c FreezeProject/timer.c -o timer.o
vga.o: FreezeProject/vga.c
	$(CC) $(CFLAGS) -c FreezeProject/vga.c -o vga.o

kernel.bin: start.o crt.o serial.o kernel.o input.o memory.o rtc.o shell.o timer.o vga.o FreezeProject/linker.ld
	$(LD) $(LDFLAGS) -T FreezeProject/linker.ld -o kernel.bin start.o crt.o serial.o kernel.o input.o memory.o rtc.o shell.o timer.o vga.o

iso/boot/grub/grub.cfg: FreezeProject/grub/grub.cfg
	mkdir -p iso/boot/grub
	cp FreezeProject/grub/grub.cfg iso/boot/grub/

freeze.iso: kernel.bin iso/boot/grub/grub.cfg
	rm -rf iso/boot/kernel.bin
	mkdir -p iso/boot/grub
	cp kernel.bin iso/boot/
	grub-mkrescue -o freeze.iso iso

clean:
	rm -f *.o kernel.bin freeze.iso
	rm -rf iso

.PHONY: all clean

