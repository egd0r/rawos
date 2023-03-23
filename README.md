# rawos

This is a repository of working documents for my implementation of a kernel in long mode.

## Pre-requisites
1. GCC >= 12.2.1
2. GNU Binutils >= 2.40
3. GNU Tar >= 1.34
4. grub-mkrescue (GRUB) 2 >= 2.06
5. QEMU emulator >= 7.2.0
6. NASM >= 2.15.05 

## Compilation instructions
If the pre-requisites above are satisfied, the following Make commands should execute fine.
### Compiling and running project
`make kernel`
### Cleaning project
`make clean`
### Starting in suspended state (debug mode)
`make kernel a=d`

## Interacting with the kernel
Interaction can be done through any VNC client. For this project, TigerVNC was used.

`vncviewer localhost:port`

*The default port is 5900 but this can vary, check the output of the Make command*