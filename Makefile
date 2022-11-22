SRCPATH = src
OBJPATH = obj
CFLAGS = -Wall -fpic -ffreestanding -fno-stack-protector -nostdinc -nostdlib -no-pie -lc

kernel:
	nasm -f elf64 src/main64.asm -o obj/main64.o
	nasm -f elf64 src/boot.asm -o obj/boot.o
	nasm -f elf64 src/mbootheader.asm -o obj/mbootheader.o

	gcc $(CFLAGS) -mno-red-zone -c $(SRCPATH)/kernel.c -o $(OBJPATH)/kernel.o

	ld -n -o kernel.bin -T linker.ld obj/main64.o obj/boot.o obj/mbootheader.o obj/kernel.o
	cp kernel.bin boot/kernel.bin
	
	grub-mkrescue /usr/lib/grub/i386-pc -o kernel.iso . 

	qemu-system-x86_64 -d int -cdrom kernel.iso

clean:
	rm obj/* kernel.bin kernel.iso
