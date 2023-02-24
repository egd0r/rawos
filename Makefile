SRCPATH = src
OBJPATH = obj
CFLAGS = -Wall -fpic -ffreestanding -fno-stack-protector -nostdinc -nostdlib -no-pie -mno-red-zone -g -Isrc/headers/ 
NASMFLAGS = -Fdwarf -g -f elf64


kernel:
	nasm -Fdwarf -g -f elf64 src/main64.asm -o obj/main64.o
	nasm -Fdwarf -g -f elf64 src/boot.asm -o obj/boot.o
	nasm -Fdwarf -g -f elf64 src/mbootheader.asm -o obj/mbootheader.o


	gcc $(CFLAGS) -c $(SRCPATH)/kernel.c -o $(OBJPATH)/kernel.o
	gcc $(CFLAGS) -c $(SRCPATH)/interrupts.c -o $(OBJPATH)/interrupts.o
	gcc $(CFLAGS) -c $(SRCPATH)/paging.c -o $(OBJPATH)/paging.o
	gcc $(CFLAGS) -c $(SRCPATH)/memory.c -o $(OBJPATH)/memory.o
	gcc $(CFLAGS) -c $(SRCPATH)/multitasking.c -o $(OBJPATH)/multitasking.o
	gcc $(CFLAGS) -c $(SRCPATH)/display.c -o $(OBJPATH)/display.o
	gcc $(CFLAGS) -c $(SRCPATH)/gdt.c -o $(OBJPATH)/gdt.o
	gcc $(CFLAGS) -c $(SRCPATH)/syscalls.c -o $(OBJPATH)/syscalls.o
	gcc $(CFLAGS) -c $(SRCPATH)/drivers/vga.c -o $(OBJPATH)/vga.o
	gcc $(CFLAGS) -c $(SRCPATH)/drivers/io.c -o $(OBJPATH)/io.o
	gcc $(CFLAGS) -c $(SRCPATH)/drivers/fs.c -o $(OBJPATH)/fs.o

	ld --no-relax -n -o kernel.bin -T linker.ld obj/*.o
	cp kernel.bin boot/kernel.bin

	# Making initrd
	tar -cf initrd.tar initrd/*
	cp initrd.tar boot/initrd.tar
	
	grub-mkrescue /usr/lib/grub/i386-pc -o kernel.iso . 

ifeq ($(a),)
	qemu-system-x86_64 -cdrom kernel.iso 
else
	qemu-system-x86_64 -s -S -cdrom kernel.iso 
endif

	# For debugging:
	# 	-s -S flags to QEMU
	# 	target remote localhost:1234 ;; remote target opened by qemu
	# 	symbol-file boot/kernel.bin ;; load symbols

show:
	qemu-system-x86_64 -cdrom kernel.iso

clean:
	rm obj/* kernel.bin kernel.iso
	rm initrd.tar
