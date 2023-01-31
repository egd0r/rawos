#pragma once

// What is a task? What to save? How are they organised?
#define MAX_TASKS_PER_BLOCK 128

#include "interrupts.h"

#define STACK_SIZE 4096
#define ONE_OVER_TWO 0x80000000000000000000

#define CLI()		asm("cli");
#define STI()		asm("sti");

extern uint64_t ms_since_boot; // Making ms_since_boot global

// Organisaion
enum TASK_STATE {
	RUNNING,
	BLOCKED,
	READY
};

typedef struct task_item_ll {
    int PID;
	enum TASK_STATE task_state;
    int parent_PID;
    int switches;
    uint8_t *stack;
	uint64_t *cr3;
	struct task_item_ll *next;
	struct task_item_ll *prev;
} TASK_LL;

extern TASK_LL *current_item; // Making current item global for TESTING purposes 

// For organisation of chunks of tasks if needed
typedef struct task_item_group {
	TASK_LL *ready_start;
	TASK_LL *ready_end;
	TASK_LL *current_item;
	TASK_LL *blocked;
	TASK_LL *terminated_curr;
} TASK_GRP;

// Process management
int create_task(void *entry_point);
int kill_task(int pid);

void lock_scheduler();
void unlock_scheduler();
TASK_LL * schedule(INT_FRAME *curr_proc_state);

TASK_LL * find_prev_task(int pid);

#define TASK(pid) find_prev_task(pid)->next


