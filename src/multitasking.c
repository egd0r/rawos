#include "headers/types.h"
#include "headers/multitasking.h"
#include "headers/memory.h"

TASK_ITEM *current_task = NULL;
TASK_ITEM *queue_end = NULL;

/*
    Kernel entry will create init process as first process with same state
    Places new task at end of queue
*/
int create_task(void *entry_point) {
    // if (entry_point == NULL && current_task == NULL); // Condition for init process 
    TASK_ITEM *new_task = new_malloc(sizeof(TASK_ITEM));

    // FILL VALUES
    new_task->PID = 0;
    new_task->switches = 0;
    new_task->next = new_task;
    new_task->prev = new_task;
    memset(&(new_task->state), 0, sizeof(CPU_STATE)); // Clear CPU_STATE

    new_task->state.rip = (uint64_t)entry_point;
    new_task->state.eflags = 0x202;
    new_task->state.cs = 0x08;

    // new_task->stack ; // Point to end

    if (current_task == NULL) {
        printf("Created a new task with PID %d\n", new_task->PID);    
        current_task = new_task;
        queue_end = new_task;
        return;
    }

    new_task->PID = current_task->prev->PID += 1;

    new_task->next = queue_end;
    new_task->prev = current_task;

    queue_end->prev = new_task;
    current_task->next = new_task;

    new_task->parent_PID = current_task->PID;

    queue_end = new_task;
    printf("Created a new task with PID %d\n", new_task->PID);    

    return;    
}


int kill_task(int pid) {
    if (pid == 0) return; // Can't kill init
}

// Simple RR scheduler
CPU_STATE *schedule(CPU_STATE curr_proc_state) {
    if (current_task == NULL) return;

    current_task->switches++;
    current_task->state = curr_proc_state;

    current_task = current_task->prev;

    return &(current_task->state);
}

void test_state() {
    printf("Testing register state;");
}
