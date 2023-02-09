#include "headers/types.h"
#include "headers/multitasking.h"
#include "headers/memory.h"

// Global variables
TASK_LL *ready_start = NULL;
TASK_LL *ready_end = NULL;

TASK_LL *blocked = NULL;

TASK_LL *current_item = NULL;

TASK_LL *terminated_curr = NULL;

uint8_t *heap_start = 0x0005000000;
uint8_t *heap_current;

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

extern uint64_t page_table_l4; // Kernel data
// Loading state into new address space
uint8_t * place_state(void * cr3, void * entry_point) {
    if (entry_point != 0x00)
        load_cr3(cr3);

    INT_FRAME new_state;
    uint8_t *stack_pos = 0x80000fff;

    stack_pos -= sizeof(INT_FRAME);
    new_state.cr3 = cr3;
    new_state.vector = 0x20;
    new_state.rip = (uint64_t)entry_point;
    new_state.eflags = 0x202;

    new_state.r15 = 0x69;

    // This code selector is what can take processor into ring 3
    new_state.cs = 0x08;
    new_state.rsp = 0x80000fff; //
    new_state.rbp = 0;

    *((INT_FRAME *)stack_pos) = new_state;

    if (entry_point != 0x00)
        load_cr3((uint64_t)(&page_table_l4)&0xFFFFF);

    return stack_pos;
}

/*
    Kernel entry will create init process as first process with same state
    Places new task at end of queue
*/
extern uint64_t page_table_l3; // Kernel data
int create_task(void *entry_point) {
    // Allocating new page for L4 table
    uint64_t *l4_pt_virt = p_alloc(PAGE_DIR_VIRT, 1);
    // // Allocating new page for L3 table
    // uint64_t *l3_pt_virt = p_alloc(PAGE_DIR_VIRT, 1);
    // // Allocating new page for L2 table
    // uint64_t *l2_pt_virt = p_alloc(PAGE_DIR_VIRT, 1);
    // // Allocating new page for L1 table
    // uint64_t *l1_pt_virt = p_alloc(PAGE_DIR_VIRT, 1);
    // // Linking L4 -> L3
    // l4_pt_virt[0] = get_pagetable_entry(l3_pt_virt);
    // // Linking L3 -> L2
    // l3_pt_virt[0] = get_pagetable_entry(l2_pt_virt);
    // // Linking L2 -> L1
    // l2_pt_virt[0] = get_pagetable_entry(l1_pt_virt);    

    // WAIT FOR SHARED MEMORY FUNCTIONALITY? Just some more paging methods
    // Creating entry for IDT virtual address in high memory

    // Creating entry for GDT virtual address in high memory

    // Getting L1 PTE of allocated page (masked physical address)

    // Get physical address of entry point - this will be hugepaged so get_pagetable_entry needs to be modified

    uint64_t l4_pt_phys = get_pagetable_entry(l4_pt_virt);
    // Self referencing
    l4_pt_virt[510] = l4_pt_phys;

    // Mapping kernel structures
    l4_pt_virt[511] = ((uint64_t)(&page_table_l3))&0xFFFFF | PRESENT | RW;

    TASK_LL *new_task = new_malloc(sizeof(TASK_LL));
    
    INT_FRAME new_state = {0};


    // new_task->stack = new_malloc(STACK_SIZE); // sbrk(STACK_SIZE);
    //Pointing stack to end
    // new_task->stack = &((new_task->stack)[STACK_SIZE-1]);
    new_task->cr3 = (uint64_t *)(l4_pt_phys & ~0xFFF);

    new_task->stack = place_state(new_task->cr3, entry_point);


    // FILL VALUES
    if (entry_point == 0x00) new_task->PID = 0;
    else new_task->PID = PID_COUNTER++;
    new_task->switches = 0;

    // // Unmasking and setting as CR3 of new process
    // new_state.cr3 = (uint64_t *)(l4_pt_phys & ~0xFFF);
    // new_state.vector = 0x20;
    // new_state.rip = (uint64_t)entry_point;
    // new_state.eflags = 0x202;

    // new_state.r15 = 0x69;

    // // This code selector is what can take processor into ring 3
    // if (entry_point == 0x00) new_state.cs = 0x08;
    // else new_state.cs = 0x08; // Test without changing CS
    // new_state.rsp = new_task->stack; //
    // new_state.rbp = 0;

    // new_task->stack -= sizeof(INT_FRAME);
    // *((INT_FRAME *)new_task->stack) = new_state;


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
TASK_LL * schedule(INT_FRAME *curr_proc_state) {
    if (ready_start == NULL)
        return NULL;


    // *(current_item->state) = *curr_proc_state; // Temp

    // This context is saved automatically
    current_item->stack = curr_proc_state; // Set to new rsp to not overwrite data

    // current_item->stack -= sizeof(CPU_STATE);
    // *((CPU_STATE *)current_item->stack) = curr_proc_state; // Saving new context

    current_item->task_state = READY;
    ready_end->next = current_item;
    ready_end = ready_end->next;
    current_item = ready_start;
    current_item->task_state = RUNNING;
    ready_start = ready_start->next; // Progress ready_start

    return current_item;
}