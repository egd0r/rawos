#pragma once
#include <types.h>
#include <multitasking.h>

typedef struct {
    char ch;
    char attribute;
}__attribute__((packed)) SCR_CHAR;

#include <vga.h>

typedef struct TASK_DISPLAY_INFO {
    int xpos;
    int ypos;
	int xmin;
	int xmax;
	int ymin;
	int ymax;
	// unsigned char *rel_video;
} TASK_DISP_INFO;



// Per process container
typedef struct CONT_O{
    int pid;
    int parent_screen_id;
    TASK_DISP_INFO display_blk;
    IN_STREAM stream;
} CONTAINER;

typedef struct SCREEN {
    SCR_CHAR chars[COLUMNS*(LINES+1)];
    int id;
    int selected_cont;
    int cont_size;
    struct CONT_O conts[4];
    struct SCREEN *next;
} SCREEN_O;

int new_disp(int curr, int pid);

int taskbar_disp(int pid);

void getch(char *buffer);

int attach_proc_to_screen(int pid, int sic);
int remove_proc_from_screen(int pid, int sic);

extern SCREEN_O * current_screen;
extern int no_screens;
// extern SCREEN_O screen_arr[COLUMNS];
extern SCREEN_O * screen_root;
