#include <types.h>
#include <vga.h>
#include <multiboot2.h>
#include <interrupts.h>
#include <paging.h>
#include <memory.h>
#include <multitasking.h>
//For testing
#include <ata.h>
#include <io.h>
#include <stdarg.h>

void sys_printf(const char *format, ...) {
    // syscal_test(4, format);
    va_list arg;
    va_start(arg, format);
    char *arg_o = va_arg(arg, char *);
    asm __volatile__("int $0x80" : : "a" (4), "b" (format), "c" (arg_o));
    va_end(arg);
}

void sys_getch(char *buffer) {
    asm __volatile__("int $0x80" : : "a" (3), "b" (buffer));
}

void sys_sleep(int secs) {
    asm __volatile__("int $0x80" : : "a" (35), "b" (secs));
}

/*
    Can force scheduler by calling interrupt 0x20 = 32 in stub table which corresponds to timer interrupt

*/
// Can align for memory safety when copying to user page table
__attribute__((aligned(0x1000))) 
void taskA() {
    // for (int i=0; i<100; i++) {
    //     i--;
    // }
    // int *test = new_malloc(sizeof(int)*5);
    // new_free(test);
    int *x = (int *)new_malloc(sizeof(int)*3);
    x[0] = 5;
    while (1) {
        // if (x[0] % 10000 == 0)
        sys_printf("A");
        // sleep(10);
        // cls();

        x[0]++;
    }
}

void taskB() {

    int *test = new_malloc(sizeof(int)*5);
    test[0] = 5;
    while(1) {
        sys_printf("B ");
        sys_sleep(1);
    }
}

void taskC() {

    while (1) {
        sys_printf("C");
    } 
}

extern uint64_t RODATA_START;
extern void syscal_test(int syscall_num, char *rbx);


void process_command(char *command);
int kmain(unsigned long mbr_addr) {
    heap_current = heap_start;
    // Initialise IDT
    asm __volatile__("mov %rsp, [stk_top]"); // Recreating stack in kmain

    idt_init();

    // Map pages? Already got 16MB identity mapped
    // Parse mb struct
    memset(BITMAP_VIRTUAL, ~0, 0x10000); //Sets bitmap to entirely allocated

    // Initialises memory map and finds initrd
    init_memory_map(mbr_addr);

    create_task(0x00);

    // Allocate at 0
    uint64_t ptr = (uint64_t)kalloc_physical(1);
    ptr = (uint64_t)kalloc_physical(1);
    ptr = (uint64_t)kalloc_physical(1);
    ptr = (uint64_t)kalloc_physical(1);
    // Free at 0
    kfree_physical(ptr);

    int *arr = new_malloc(sizeof(int)*5);
    arr[1] = 5;

    *((int *)0xb8900) = mbr_addr;  
    
    create_task(&taskA);
    create_task(&taskB);
    create_task(&taskC);
    create_task(&k_taskbar);
    cls();
    activate_interrupts();

    // Shell
    char command_buffer[50] = {'\0'};
    char *temp = command_buffer;

    sys_printf(">");
    while (1) {
    //   printf("k2");
        sys_getch(temp);
        char ch = temp[0];
        if (ch == '\n') {
            *(temp)='\0';
            process_command(command_buffer);
            sys_printf(">");
            temp = command_buffer;
        } else if (ch != -1) {
            sys_printf("%c ", ch);
            *temp = ch; temp++;
        }

        // sys_getch(temp);
        // // if (ch == '\0') get_word();
        // if (temp[0] != -1) {
        //     sys_printf("%c ", *temp);
        // }
    }; //Spin on hang
    // Spawn init process
    
    return 0;
}

int strncmp(char *s, char *t, int num) {
   for ( ; num >0;  s++, t++, num--)
        if (*s == 0)
            return 0;

    if (*s == *t) {
        ++s;
        ++t;
    }
    else if (*s != *t)
        return *s - *t;   

    return 1;
}


void print_proc(TASK_LL *task) {
    sys_printf("PID: %d    ", task->PID);
    sys_printf("Proc time(ms): %d    ", task->proc_time);
    sys_printf("Screen ID: %d\n", task->screen_id);
}

void ps() {
    sys_printf("\nCurrent:\n");
    print_proc(current_item);
    sys_printf("Ready:\n");
    for (TASK_LL *temp = ready_start; temp != NULL; temp=temp->next) {
        print_proc(temp);
        if (temp == ready_end) break;
    }
    sys_printf("Blocked:\n");
    for (TASK_LL *temp = blocked_start; temp != NULL; temp=temp->next) {
        print_proc(temp);
        if (temp == blocked_end) break;
    }
}

void clear() {
    cls();
}

void process_command(char *command) {
    if (strncmp(command, "ps", 2) == 0) ps();
    else if (strncmp(command, "clear", 5) == 0) clear();
}
