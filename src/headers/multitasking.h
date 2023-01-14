#pragma once

// What is a task? What to save? How are they organised?

typedef struct cpu_state {
    uint64_t rsp;
    uint64_t rflags;
    uint64_t cs;
    uint64_t rip;
    uint64_t rdi;
	uint64_t rsi;
	uint64_t rdx;
	uint64_t rcx;
	uint64_t rbx;
	uint64_t rax;
    uint64_t eflags;
} CPU_STATE;

// Organisaion
typedef struct task_queue {
    int PID;
    CPU_STATE state;
    int parent_PID;
    int switches;
    struct task_queue *next;
    struct task_queue *prev;
} __attribute__((packed)) TASK_ITEM;


// Process management
int create_task(void *entry_point);
int kill_task(int pid);

CPU_STATE *schedule(CPU_STATE curr_proc_state);

void test_state();


