SRCPATH = src
OBJPATH = obj
CFLAGS = -Wall -fpic -ffreestanding -fno-stack-protector -nostdinc -nostdlib -no-pie -lc -g

kernel:
	nasm -g -f elf64 src/main64.asm -o obj/main64.o
	nasm -g -f elf64 src/boot.asm -o obj/boot.o
	nasm -g -f elf64 src/mbootheader.asm -o obj/mbootheader.o

	gcc $(CFLAGS) -mno-red-zone -c $(SRCPATH)/kernel.c -o $(OBJPATH)/kernel.o

	ld -n -o kernel.bin -T linker.ld obj/main64.o obj/boot.o obj/mbootheader.o obj/kernel.o
	cp kernel.bin boot/kernel.bin
	
	grub-mkrescue /usr/lib/grub/i386-pc -o kernel.iso . 

	qemu-system-x86_64 -s -S -cdrom kernel.iso

	# For debugging:
	# 	-s -S flags to QEMU
	# 	target remote localhost:1234 ;; remote target opened by qemu
	# 	symbol-file boot/kernel.bin ;; load symbols

clean:
	rm obj/* kernel.bin kernel.iso
