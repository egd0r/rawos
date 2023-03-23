#include <types.h>
#include <multitasking.h>
#include <memory.h>

#include <vga.h>

#define VIDEO_MEM_PAGES 1

// Global variables


int current_display = 0;

TASK_GRP init_task_grp = {0};

uint8_t *heap_start = (uint8_t *)0x0000000050000000;
uint8_t *heap_current;

int PID_COUNTER = 2;
int irq_disables = 0;

extern uint64_t page_table_l4; // Kernel data
extern void load_cr3(uint64_t pt);
uint16_t poll_pit();
TASK_LL * sleep(int secs) {
    // Add current process to blocked queue
    load_cr3((uint64_t)(&page_table_l4)&0xFFFFF);
    current_item->wake_after_ms = secs*1000;
    current_item->proc_time += poll_pit()/PIT_RELOAD;
    current_item->task_state = BLOCKED;

    // Add seconds to sleep for
    if (blocked_start == NULL) {
        blocked_start = current_item;
        blocked_end = current_item;
        blocked_start->next = blocked_end;
    } else {
        current_item->next = blocked_end->next;
        current_item->prev = blocked_end;
        blocked_end->next = current_item;
        blocked_end = current_item;
        blocked_start->prev = blocked_end;
        blocked_end->next = blocked_start;
    }
    // Schedule next process from ready queue

    current_item = ready_start;
    current_item->task_state = RUNNING;
    ready_start = ready_start->next; // Progress ready_start
    return current_item;
}

extern void remove_display(int sid);
extern TASK_DISP_INFO NEW_FULL_DISPLAY();
extern SCREEN_O *find_screen(int id);
void kill_task(int pid) {
    // Add current process to blocked queue
    load_cr3((uint64_t)(&page_table_l4)&0xFFFFF);

    // Add seconds to sleep for
    TASK_LL *to_kill = TASK(pid);
    if (to_kill == NULL || to_kill == current_item) return;
    
    if (to_kill->task_state == READY) {
        TASK_LL *prev = NULL;
        for (TASK_LL *ready = ready_start; ready != ready_end && ready != to_kill; ready = ready->next) {
            prev = ready;
        }

        if (prev == NULL) {
            if (to_kill->next == to_kill) {
                ready_start = NULL;
                ready_end = NULL;
            } else {
                to_kill->prev->next = ready_start->next;
                to_kill->next->prev = ready_start->prev;
                ready_start = ready_start->next;
            }
        } else {
            if (ready_end == to_kill) {
                ready_end = prev;
                prev->next = NULL;
            } else {
                prev->next = to_kill->next;
            }
        }
    } else if (to_kill->task_state == BLOCKED) {
        TASK_LL *prev = NULL;
        for (TASK_LL *ready = blocked_start; ready != blocked_end && ready != to_kill; ready = ready->next) {
            prev = ready;
        }

        if (prev == NULL) {
            if (to_kill->next == to_kill) {
                blocked_start = NULL;
                blocked_end = NULL;
            } else {
                to_kill->prev->next = blocked_start->next;
                to_kill->next->prev = blocked_start->prev;
                blocked_start = blocked_start->next;
            }
        } else {
            if (blocked_end == to_kill) {
                blocked_end = prev;
                prev->next = NULL;
            } else {
                prev->next = to_kill->next;
            }
        }
    } else {
        return;
    }
   


    // Adding to killed list
    to_kill->next = NULL;
    to_kill->prev = NULL;
    if (terminated_curr != NULL) {
        terminated_curr->prev = to_kill;
        to_kill->next = terminated_curr;
    }
    terminated_curr = to_kill;

    // Removing screen
    SCREEN_O *scr = find_screen(to_kill->screen_id);

    int i=0;
    for (; i<scr->cont_size && scr->conts[i].pid != to_kill->PID; i++);

    for (i++; i<scr->cont_size; i++) {
        scr->conts[i] = scr->conts[i-1];
    }

    scr->cont_size--;

    if (scr->cont_size <= 1) {
        remove_display(scr->id);
    } else {
        for (int i=0; i<scr->cont_size; i++) {
            if (scr->conts[i].pid != 2) scr->conts[i].display_blk = NEW_FULL_DISPLAY();
        }
    }

}

// Loading state into new address space
uint8_t * place_state(void * cr3, void * entry_point, TASK_LL *new_task) {
    // Getting physical address of mapping
    uint64_t new_task_phys = (uint64_t)get_pagetable_entry((uint64_t)new_task);
    // Place this within tasks address space
    // Cause PF to allocate new space

    // Load address space of new task
    load_cr3((uint64_t)cr3);
    // *((uint64_t *)(PROCESS_CONT_ADDR)) = 0; // Allocating page by causing a page fault
    // Accessing allocated page
    // *((uint64_t *)PAGE_DIR_VIRT)
    uint64_t *proc_space = ((uint64_t *)0xffffff0000028000);
    proc_space[1] = new_task_phys; //Mapping to own process context done here

    INT_FRAME new_state = {0};
    uint8_t *stack_pos = (uint8_t *)0x0000000080000ff0;

    stack_pos -= sizeof(INT_FRAME);
    new_state.cr3 = (uint64_t)cr3;
    new_state.vector = 0x20;
    new_state.rip = (uint64_t)entry_point;
    new_state.eflags = 0x202;

    new_state.r15 = 0x69;

    // This code selector is what can take processor into ring 3
    new_state.cs = 0x08;
    new_state.rsp = 0x80000ff0; //
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
int create_task(void *entry_point, int sid) {
    // Allocating new page for L4 table
    
    // Assert screen create

    TASK_LL *new_task = (TASK_LL *)kp_alloc(1);

    if (entry_point != 0x00) {
        uint64_t *l4_pt_virt = (uint64_t *)p_alloc(PAGE_DIR_VIRT, 1);
        uint64_t l4_pt_phys = (uint64_t)get_pagetable_entry((uint64_t)l4_pt_virt);
        // Self referencing
        l4_pt_virt[510] = l4_pt_phys;
        // Mapping kernel structures
        l4_pt_virt[511] = (((uint64_t)(&page_table_l3))&0xFFFFF) | PRESENT | RW;
        new_task->cr3 = (uint64_t)(l4_pt_phys & ~0xFFF);
    } else {
        new_task->cr3 = (uint64_t)((uint64_t)(&page_table_l4)&0xFFFFF);
    }
    
    new_task->stack = place_state((void *)new_task->cr3, entry_point, new_task);


    // FILL VALUES
    // Place important structures inside 'user space'
    if (entry_point == 0x00) {
        new_task->PID = 1;
        new_task->flags |= DISPLAY_TRUE;
        new_task->heap_current = heap_current + ((VIDEO_MEM_PAGES+PROC_PAGE_SIZE) << 12);
    } else { 
        new_task->PID = PID_COUNTER++;
        new_task->heap_current = ((uint8_t *)HEAP_START) + ((VIDEO_MEM_PAGES+PROC_PAGE_SIZE) << 12); // Add one to page index to make space for video output
    }
    new_task->switches = 0;
    
    if (entry_point == k_taskbar) new_task->screen_id = taskbar_disp(new_task->PID);
    else {
        new_task->screen_id = new_disp(sid, new_task->PID);
    }

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

    kprintf("Created a new task with PID %d\n", new_task->PID);    
    init_task_grp.number_of_tasks++;

    return 1;    
}

// Only available to interrupts
TASK_LL * TASK(int pid) {
    if (pid == current_item->PID) return current_item;

    TASK_LL *prev = NULL;
    for (TASK_LL *temp = ready_start; prev != ready_end; temp=temp->next) {
        if (temp->PID == pid) return temp;
        prev = temp;
    }
    
    TASK_LL *prev_blk = NULL;
    for (TASK_LL *temp = blocked_start; prev_blk != blocked_end; temp=temp->next) {
        if (temp->PID == pid) return temp;
        prev_blk = temp;
    }

    return NULL;
}

TASK_LL * find_prev_task(int pid) {
    for (TASK_LL *temp = current_item; temp->PID != current_item->PID; temp=temp->next) {
        if (temp->next->PID == pid) {
            return temp;
        }
    }
    return NULL;
}

// Simple RR scheduler
TASK_LL * schedule(INT_FRAME *curr_proc_state) {

    current_item->heap_current = heap_current;
    // *(current_item->state) = *curr_proc_state; // Temp

    // This context is saved automatically
    current_item->stack = (uint8_t *)curr_proc_state; // Set to new rsp to not overwrite data

    // current_item->stack -= sizeof(CPU_STATE);
    // *((CPU_STATE *)current_item->stack) = curr_proc_state; // Saving new context

    if (ready_start != NULL) {
        current_item->task_state = READY;
        ready_end->next = current_item;
        ready_end = ready_end->next;
        current_item = ready_start;
        current_item->task_state = RUNNING;
        ready_start = ready_start->next; // Progress ready_start

    }

    heap_current = current_item->heap_current;
    current_item->switches++;

    return current_item;
}