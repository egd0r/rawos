SRCPATH = src
OBJPATH = obj


kernel:
	nasm -f elf64 src/main64.asm -o obj/main64.o
	nasm -f elf64 src/boot.asm -o obj/boot.o
	nasm -f elf64 src/mbootheader.asm -o obj/mbootheader.o

	ld -n -nopie -o kernel.bin -T linker.ld obj/main64.o obj/boot.o obj/mbootheader.o
	cp kernel.bin boot/kernel.bin
	
	grub-mkrescue /usr/lib/grub/i386-pc -o kernel.iso . 

	qemu-system-x86_64 -cdrom kernel.iso

clean:
	rm obj/* kernel.bin kernel.iso
