ASM = nasm
CC = i686-linux-gnu-gcc
CXX := $(shell command -v i686-linux-gnu-g++ 2>/dev/null || command -v g++ 2>/dev/null || echo i686-linux-gnu-g++)
LD = i686-linux-gnu-ld

CFLAGS = -m32 -ffreestanding -nostdlib -fno-exceptions -fno-rtti

all: os-image.bin

boot.o: boot/boot.asm
	$(ASM) -f bin boot/boot.asm -o boot.bin

entry.o: kernel/entry.s
	$(ASM) -f elf32 kernel/entry.s -o entry.o

kernel.o: kernel/kernel.cpp
	$(CXX) $(CFLAGS) -c kernel/kernel.cpp -o kernel.o

kernel.bin: entry.o kernel.o kernel/linker.ld
	$(LD) -m elf_i386 -T kernel/linker.ld entry.o kernel.o -o kernel.bin

os-image.bin: boot.o kernel.bin
	cat boot.bin kernel.bin > os-image.bin

run: kernel.bin
	qemu-system-i386 -kernel kernel.bin -m 32 -serial stdio

clean:
	rm -f *.o *.bin

