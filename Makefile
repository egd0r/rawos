SRCPATH = src
OBJPATH = obj
CFLAGS = -Wall -fpic -ffreestanding -fno-stack-protector -nostdinc -nostdlib -no-pie -mno-red-zone -g
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

	ld --no-relax -n -o kernel.bin -T linker.ld obj/*.o
	cp kernel.bin boot/kernel.bin
	
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
