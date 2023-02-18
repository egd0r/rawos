#include <syscalls.h>
#include <vga.h>
#include <interrupts.h>
#include <types.h>

void * syscall_handler(INT_FRAME frame) {
    kprintf("ello there chappy\n");
    kprintf("Syscall %d with arg %d\n", frame.rax, frame.rbx);

    switch (frame.rax) {
        case 4:
            printf(frame.rbx);
    }

    return NULL;

}

// void _exit() {
//     return;
// }

// int close(int file) {
//     return -1;
// }

// // extern int errno;

// char *__env[1] = { 0 };
// char **environ = __env;

// int execve(char *name, char **argv, char **env) {
// //   errno = ENOMEM;
//   return -1;
// }


// int fork(void) {
// //   errno = EAGAIN;
//   return -1;
// }

// // int fstat(int file, struct stat *st) {
// // //   st->st_mode = S_IFCHR;
// //   return 0;
// // }

// int getpid(void) {
//   return 1;
// }

// int isatty(int file) {
//   return 1;
// }

// int kill(int pid, int sig) {
// //   errno = EINVAL;
//   return -1;
// }

// int link(char *old, char *new) {
// //   errno = EMLINK;
//   return -1;
// }

// int lseek(int file, int ptr, int dir) {
//   return 0;
// }

// int open(const char *name, int flags, int mode) {
//   return -1;
// }

// int read(int file, char *ptr, int len) {
//   return 0;
// }

// // sbrk goes here

// // int stat(char *file, int *st) {
// // //   st->st_mode = S_IFCHR;
// //   return 0;
// // }

// // int times(struct tms *buf) {
// //   return -1;
// // }

// int unlink(char *name) {
// //   errno = ENOENT;
//   return -1; 
// }

// int wait(int *status) {
// //   errno = ECHILD;
//   return -1;
// }

// int write(int file, char *ptr, int len) {
//   int todo;

//   for (todo = 0; todo < len; todo++) {
//     outbyte (*ptr++);
//   }
//   return len;
// }