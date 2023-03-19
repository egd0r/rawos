#pragma once

// What is a task? What to save? How are they organised?
#define MAX_TASKS_PER_BLOCK 128

#include <interrupts.h>
#include <io.h>

#define HEAP_START 0x5000000
#define STACK_SIZE 4096
#define ONE_OVER_TWO 0x80000000000000000000

#define PROC_PAGE_SIZE 1
#define PROCESS_CONT_ADDR (HEAP_START + (PROC_PAGE_SIZE << 12))

#define CLI()		asm("cli");
#define STI()		asm("sti");

extern uint64_t ms_since_boot; // Making ms_since_boot global

// Flags
#define DISPLAY_TRUE 0x01

// Organisaion
enum TASK_STATE {
	RUNNING,
	BLOCKED,
	READY
};



typedef struct task_item_ll {
    int PID;
	int flags;
	enum TASK_STATE task_state; // Can check state
    int parent_PID;
    int switches;
	int wake_after_ms;
	uint64_t proc_time;
    uint8_t *stack;
	uint8_t *heap_current;
	uint64_t cr3;
	int screen_id;
	struct task_item_ll *next;
	struct task_item_ll *prev;
} TASK_LL;

// For organisation of chunks of tasks if needed
typedef struct task_item_group {
	int number_of_tasks;

	TASK_LL *rdy_start;
	TASK_LL *rdy_end;

	TASK_LL *curr_item;
	
	TASK_LL *blkd_start;
	TASK_LL *blkd_end; 

	TASK_LL *trm_curr;
} TASK_GRP;

// Process management
int create_task(void *entry_point, int sid);
void kill_task(int pid);

void lock_scheduler();
void unlock_scheduler();
TASK_LL * schedule(INT_FRAME *curr_proc_state);

TASK_LL * find_prev_task(int pid);
TASK_LL * TASK(int pid);

TASK_LL * sleep(int secs);


extern TASK_GRP init_task_grp;

#define current_item init_task_grp.curr_item
#define ready_start init_task_grp.rdy_start
#define ready_end init_task_grp.rdy_end
#define blocked_start init_task_grp.blkd_start
#define blocked_end init_task_grp.blkd_end
#define terminated_curr init_task_grp.trm_curr
#define TASK_COUNT init_task_grp.number_of_tasks


// extern TASK_LL *current_item; // Making current item global for TESTING purposes 
extern int current_display; // Making current item global for TESTING purposes 
extern uint8_t *heap_start; // HERE FOR TESTING PURPOSES, should be per process
extern uint8_t *heap_current; // HERE FOR TESTING PURPOSES, should be per process
