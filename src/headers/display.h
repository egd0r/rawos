#include <types.h>
#include <multitasking.h>
#include <vga.h>

typedef struct {    
    int id;
    screen *root_screen;
    TASK_LL procs[4];
} CONTAINER;


int create_screen();
CONTAINER *remove_screen(int container_id);

void attach_proc_to_screen(TASK_LL *proc, int container_id);
void remove_proc_from_screen(TASK_LL *proc, int container_id);

