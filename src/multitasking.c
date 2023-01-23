#include "headers/types.h"
#include "headers/multitasking.h"
#include "headers/memory.h"

// Global variables
TASK_LL *ready_start = NULL;
TASK_LL *ready_end = NULL;

TASK_LL *blocked = NULL;

TASK_LL *current_item = NULL;

TASK_LL *terminated_curr = NULL;

int PID_COUNTER = 0;
int irq_disables = 0;

void lock_scheduler() {
    CLI();
    irq_disables++;
}

void unlock_scheduler() {
    irq_disables--;
    if (irq_disables == 0) {
        STI();
    }
}

void add_to_circular_q(TASK_LL *current, TASK_LL * to_add) {
    if (current == NULL) {
        current = to_add;
        to_add->next = to_add;
        to_add->prev = to_add;
        return;
    }

    to_add->prev = current->prev;
    to_add->next = current;

    current->prev->next = to_add;
    current->prev = to_add;
}

TASK_LL * remove_from_circular_q(TASK_LL * to_remove) {
    if (to_remove->next == to_remove) return NULL; // Only element in circular q

    to_remove->next->prev = to_remove->prev;
    to_remove->prev->next = to_remove->next;

    return to_remove;
}

void add_to_end(TASK_LL *start, TASK_LL *end, TASK_LL * to_add) {
    if (start == NULL) {
        start = to_add;
        end = to_add;
        to_add->next = to_add;
        to_add->prev = to_add;
        return;
    }
    to_add->prev = end;
    to_add->next = NULL;

    end->next = to_add;
    end = to_add;
}

TASK_LL * remove_from_end(TASK_LL *start, TASK_LL *end, TASK_LL *to_remove) {
    if (to_remove == start && to_remove == end) {
        start = NULL;
        end = NULL;

        return to_remove;
    }

    if (start != to_remove) {
        to_remove->prev = to_remove->next;
    } else {
        start = start->next;
    }
    
    if (end != to_remove) {
        to_remove->next = to_remove->prev;
    } else {
        end = end->next;
    }

    return to_remove;
}

/*
    Kernel entry will create init process as first process with same state
    Places new task at end of queue
*/
int create_task(void *entry_point) {
    TASK_LL *new_task = new_malloc(sizeof(TASK_LL));
    CPU_STATE *new_state = new_malloc(sizeof(CPU_STATE));

    new_task->stack = new_malloc(STACK_SIZE); // sbrk(STACK_SIZE);

    // FILL VALUES
    new_task->state = new_state;
    new_task->PID = PID_COUNTER++;
    new_task->switches = 0;
    memset(new_state, 0, sizeof(CPU_STATE)); // Clear CPU_STATE

    new_task->state->rip = (uint64_t)entry_point;
    new_task->state->eflags = 0x202;
    new_task->state->cs = 0x08;
    new_task->state->rsp = &((new_task->stack)[STACK_SIZE-1]);
    new_task->state->rbp = 0;

    new_task->next = new_task;
    new_task->prev = new_task;

    if (current_item == NULL) {
        new_task->task_state = RUNNING;
        current_item = new_task;
    } else {
        if (ready_start == NULL) {
            ready_start = new_task;
            ready_end = new_task;
        }
        new_task->task_state = READY;
        ready_end->next = new_task;
        ready_end = ready_end->next;
    }

    // new_task->stack ; // Point to end

    printf("Created a new task with PID %d\n", new_task->PID);    

    return;    
}

TASK_LL * find_prev_task(int pid) {
    for (TASK_LL *temp = current_item; temp=temp->next; temp->PID != current_item->PID) {
        if (temp->next->PID == pid) {
            return temp;
        }
    }
    return NULL;
}

// This should call schedule and get a new task - trigger different schedule interrupt? Have asm routine for switching tasks?
int block_task(int pid) {
    TASK_LL *task = TASK(pid);
    task->task_state = BLOCKED;

    // Remove from ready
    remove_from_end(ready_start, ready_end, task);
    // Add to blocked
    add_to_circular_q(blocked, task);
}


int kill_task(int pid) {
    if (pid == 0) return 1; // Can't kill init

    // Clearing memory here... closing files and all that
    
    if (current_item->PID == pid)
        return 1; // Only called from interrupt?

    TASK_LL *prev = find_prev_task(pid);
    if (prev == NULL) return 1;

    prev->next = prev->next->next;

    return 0;
}

// Simple RR scheduler
TASK_LL * schedule(CPU_STATE curr_proc_state) {
    if (ready_start == NULL)
        return NULL;

    *(current_item->state) = curr_proc_state;

    current_item->task_state = READY;
    ready_end->next = current_item;
    ready_end = ready_end->next;
    current_item = ready_start;
    current_item->task_state = RUNNING;
    ready_start = ready_start->next; // Progress ready_start

    return current_item;
}