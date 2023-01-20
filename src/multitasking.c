#include "headers/types.h"
#include "headers/multitasking.h"
#include "headers/memory.h"

TQ_STRUCT *initial_struct = NULL;

void init_tasks() {
    initial_struct = (TQ_STRUCT *)new_malloc(sizeof(TQ_STRUCT *));
    initial_struct->size = 0;
    initial_struct->current_task = -1;
}

/*
    Kernel entry will create init process as first process with same state
    Places new task at end of queue
*/
int create_task(void *entry_point) {
    // if (entry_point == NULL && current_task == NULL); // Condition for init process 
    if (initial_struct == NULL) init_tasks();

    TASK_ITEM *new_task = new_malloc(sizeof(TASK_ITEM));
    CPU_STATE *new_state = new_malloc(sizeof(CPU_STATE));

    new_task->stack = new_malloc(STACK_SIZE); // sbrk(STACK_SIZE);

    initial_struct->tasks[initial_struct->size++] = new_task;

    // FILL VALUES
    new_task->state = new_state;
    new_task->PID = initial_struct->size;
    new_task->switches = 0;
    memset(new_state, 0, sizeof(CPU_STATE)); // Clear CPU_STATE

    new_task->state->rip = (uint64_t)entry_point;
    new_task->state->eflags = 0x202;
    new_task->state->cs = 0x08;
    new_task->state->rsp = &((new_task->stack)[STACK_SIZE-1]);
    new_task->state->rbp = 0;

    // new_task->stack ; // Point to end

    printf("Created a new task with PID %d\n", new_task->PID);    

    return;    
}


int kill_task(int pid) {
    if (pid == 0) return; // Can't kill init
}

// Simple RR scheduler
TASK_ITEM * schedule(CPU_STATE curr_proc_state) {
    if (initial_struct->size <= 0) { // Returns current state if no tasks are defined
        return NULL;
    }

    // Save the state of the current task context
    if (initial_struct->current_task >= 0) {
        *(initial_struct->tasks[initial_struct->current_task]->state) = curr_proc_state;
        initial_struct->tasks[initial_struct->current_task]->switches++;
    }
        

    // Increase current count or restart    
    if (++(initial_struct->current_task) >= initial_struct->size)
        initial_struct->current_task %= initial_struct->size;

    // Return context of new one
    return (initial_struct->tasks[initial_struct->current_task]);
}

void test_state() {
    printf("Testing register state;");
}

void disable_interrupts() {
    asm("cli");
}
