#pragma once
#include <types.h>
#include <multitasking.h>
#include <vga.h>

typedef struct TASK_DISPLAY_INFO {
    int xpos;
    int ypos;
	int xmin;
	int xmax;
	int ymin;
	int ymax;
	unsigned char *rel_video;
} TASK_DISP_INFO;

typedef struct {
    char ch;
    char attribute;
}__attribute__((packed)) SCR_CHAR;

// Per process container
typedef struct CONT_O{
    int pid;
    int parent_screen_id;
    TASK_DISP_INFO display_blk;
    struct CONT_O *next;
} CONTAINER;

typedef struct SCREEN {
    SCR_CHAR chars[COLUMNS*LINES];
    int id;
    int selected_cont;
    int cont_size;
    struct CONT_O conts[4];
    IN_STREAM stream;
}__attribute__((packed)) SCREEN_O;

int new_disp(int curr, int pid, int xmin, int xmax, int ymin, int ymax);

void attach_proc_to_screen(TASK_LL *proc, int container_id);
void remove_proc_from_screen(TASK_LL *proc, int container_id);

extern int current_screen;
extern int no_screens;
extern SCREEN_O screen_arr[COLUMNS];
