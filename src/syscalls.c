#include <syscalls.h>
#include <vga.h>
#include <interrupts.h>
#include <types.h>

extern void load_cr3(uint64_t pt);
void syscall_handler(INT_FRAME **frame) {
    // kprintf("ello there chappy\n");
    // kprintf("Syscall %d with arg %d\n", frame.rax, frame.rbx);

    switch ((*frame)->rdx) {

        case 3:
            getch((char *)(*frame)->rbx);
            break;
        case 4:
            printf((const char *)(*frame)->rbx);
            break;
        case 35:
            break;

    }

    if ((*frame)->rdx == 35) {
        TASK_LL *new_task = sleep((*frame)->rbx);
        *frame = (void *)(new_task->stack);
        load_cr3(new_task->cr3);
    }

}
