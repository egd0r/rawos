#include <types.h>
#include <vga.h>
#include <multiboot2.h>
#include <interrupts.h>
#include <paging.h>
#include <memory.h>
#include <multitasking.h>
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
        sys_sleep(5);
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

    create_task(0x00, -1);

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
    
    create_task(&k_taskbar, -1);
    create_task(&taskA, -1);
    create_task(&taskB, -1);
    create_task(&taskB, -1);
    create_task(&taskC, -1);
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

int strncmp(const char* s1, const char* s2, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        if (s1[i] != s2[i]) {
            return s1[i] - s2[i];
        } else if (s1[i] == '\0') {
            return 0;
        }
    }
    return 0;
}


void print_proc(TASK_LL *task) {
    sys_printf("PID: %d    ", task->PID, " ");
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
    sys_printf("Terminated:\n");
    for (TASK_LL *temp = terminated_curr; temp != NULL; temp=temp->next) {
        print_proc(temp);
    }
    sys_printf("\n");
}

void clear() {
    cls();
}

char * progress_until_char(char *str, char find) {
    for (; *str!=find; str++) {
        if (*str == '\0') return 0;
    }
    return str;
}

void create(char *command) {
    char *task_req = 0x00;
    if ((task_req = progress_until_char(command, ' ')) == 0) return;
    task_req++;
    char *screen = 0x00;
    if ((screen = progress_until_char(task_req, ' ')) == 0) return;
    *screen = '\0';
    screen++;
    int sid = atoi(screen);

    // sys_printf("New task to make %s\n", taskA);
    // sys_printf("New screen to add to %s\n", screen);

    CLI();
    if (strncmp(task_req, "a", 1) == 0) {
        create_task(&taskA, sid);
    } else if (strncmp(task_req, "b", 1) == 0) {
        create_task(&taskB, sid);
    }
    else if (strncmp(task_req, "c", 1) == 0) {
        create_task(&taskC, sid);
    } else printf("Task unrecognised.\n");
    STI();
}

void help() {
    sys_printf("\n");
    sys_printf("Commands available:\n");
    sys_printf("    ps    - List active processes\n");
    sys_printf("    clear - Clear screen\n");
    sys_printf("    help  - Print this screen\n");
    sys_printf("    create- Create a new process Args: proc, screen\n");
    sys_printf("            Available processes: A, B, C\n");
    sys_printf("    kill  - Kill a process Args: pid\n");
    sys_printf("\n");
}

void kill(char *command) {
    char *pid = 0x00;
    if ((pid = progress_until_char(command, ' ')) == 0) return;
    pid++;
    int pid_kill = atoi(pid);
    
    CLI();
    kill_task(pid_kill);
    STI();
}

void process_command(char *command) {
    sys_printf("\n");
    if (strncmp(command, "ps", 2) == 0) ps();
    else if (strncmp(command, "kill", 4) == 0) kill(command);
    else if (strncmp(command, "help", 4) == 0) help();
    else if (strncmp(command, "clear", 5) == 0) clear();
    else if (strncmp(command, "create", 6) == 0) create(command);

    else sys_printf("Unrecognised command: %s\n", command);
}
