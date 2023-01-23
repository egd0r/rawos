#pragma once

// What is a task? What to save? How are they organised?
#define MAX_TASKS_PER_BLOCK 128

typedef struct cpu_state {
	uint64_t r15;
	uint64_t r14;
	uint64_t r13;
	uint64_t r12;
	uint64_t rbx;
	uint64_t rbp;
	uint64_t rip;
	uint64_t cs;
	uint64_t eflags;
	uint64_t rsp;
} CPU_STATE;

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
    CPU_STATE * state;
    int parent_PID;
    int switches;
    uint8_t *stack;
	struct task_item_ll *next;
	struct task_item_ll *prev;
} TASK_LL;

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
TASK_LL * schedule(CPU_STATE curr_proc_state);

TASK_LL * find_prev_task(int pid);

#define TASK(pid) find_prev_task(pid)->next


