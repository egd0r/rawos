#include "headers/types.h"
#include "headers/multitasking.h"
#include "headers/memory.h"

TASK_ITEM *current_task = NULL;

/*
    Kernel entry will create init process as first process with same state
    Places new task at end of queue
*/
int create_task(void *entry_point) {
    TASK_ITEM *new_task = new_malloc(sizeof(TASK_ITEM));

    // FILL VALUES
    new_task->PID = 0;
    new_task->next = new_task;
    new_task->prev = new_task;
    memset(&(new_task->state), 0, sizeof(CPU_STATE)); // Clear CPU_STATE
    new_task->state.rip = entry_point;
    
    new_task->state.eflags = 0x202;
    new_task->state.cs = 0x08;

    if (current_task == NULL) {
        current_task = new_task;
        return;
    }

    new_task->prev = current_task->prev;
    new_task->next = current_task;

    current_task->prev->next = new_task;

    new_task->PID = current_task->prev->PID += 1;

    new_task->parent_PID = current_task->PID;

    printf("Created a new task with PID %d\n", new_task->PID);    

    return;    
}


int kill_task(int pid) {
    if (pid == 0) return; // Can't kill init
}

// Simple RR scheduler
CPU_STATE *schedule(CPU_STATE curr_proc_state) {
    if (current_task == NULL) return;

    current_task->state = curr_proc_state;

    current_task = current_task->next;

    return &(current_task->state);
}

