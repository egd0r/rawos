#include <syscalls.h>
#include <vga.h>
#include <interrupts.h>
#include <types.h>

extern void load_cr3(uint64_t pt);
void syscall_handler(INT_FRAME **frame) {
   switch ((*frame)->rax) {
        case 3:
            getch((char *)(*frame)->rbx);
            break;
        case 4:
            printf((const char *)(*frame)->rbx);
            break;
        case 35:
            TASK_LL *new_task = sleep((*frame)->rbx);
            load_cr3(new_task->cr3);
            *frame = (void *)(new_task->stack);
            break;
    }
}
