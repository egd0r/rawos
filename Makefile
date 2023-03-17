SRCPATH = src
OBJPATH = obj
CFLAGS = -Wall -fpic -ffreestanding -fno-stack-protector -nostdinc -nostdlib -no-pie -mno-red-zone -g -Iinclude/ 
NASMFLAGS = -Fdwarf -g -f elf64

SRCFILES := $(wildcard $(SRCPATH)/*.c $(SRCPATH)/drivers/*.c)
ASMFILES := $(wildcard $(SRCPATH)/arch/x86/*.asm)

ASM_OBJ := $(patsubst $(SRCPATH)/arch/x86/%.asm,$(OBJPATH)/%.o,$(ASMFILES))

kernel: $(ASM_OBJ)
	gcc $(CFLAGS) -c $(SRCFILES)
	mv *.o $(OBJPATH)
	ld --no-relax -n -o kernel.bin -T src/linker.ld $(OBJPATH)/*.o
	cp kernel.bin boot/kernel.bin
	tar -cf initrd.tar initrd/*
	cp initrd.tar boot/initrd.tar
	grub-mkrescue /usr/lib/grub/i386-pc -o kernel.iso . 
ifeq ($(a),)
	qemu-system-x86_64 -cdrom kernel.iso 
else
	qemu-system-x86_64 -s -S -cdrom kernel.iso 
endif
show:
	qemu-system-x86_64 -cdrom kernel.iso
clean:
	rm obj/* kernel.bin kernel.iso
	rm initrd.tar

$(OBJPATH)/%.o: $(SRCPATH)/arch/x86/%.asm
	nasm $(NASMFLAGS) $< -o $@
