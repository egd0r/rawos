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

// Organisaion
typedef struct task_queue {
    int PID;
    CPU_STATE * state;
    int parent_PID;
    int switches;
    uint8_t *stack;
} TASK_ITEM;

typedef struct task_queue_struct {
    int current_task;
    int size;
    TASK_ITEM *tasks[MAX_TASKS_PER_BLOCK];
} TQ_STRUCT;

// Process management
int create_task(void *entry_point);
int kill_task(int pid);

TASK_ITEM * schedule(CPU_STATE curr_proc_state);

void test_state();


