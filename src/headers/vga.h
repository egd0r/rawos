#pragma once

#define KERNEL_VIRT 0xFFFFFFFF80000000
#define VIDEO_ACTUAL 0xFFFFFFFF800b8000 // Using vid base+virt since we're now in higher half ;)
#define VIDEO 0xFFFFFFFF800b8000 // Using vid base+virt since we're now in higher half ;)
#define PROC_VIDEO HEAP_START
#include <types.h>
static volatile uint16_t *video = (uint16_t *)VIDEO;

#define COLUMNS 80
#define LINES 24

#define VIDEO_END VIDEO + ((COLUMNS + LINES * LINES) * 2)

#define VIDEO_SIZE VIDEO_END - VIDEO

#define ATT_BLACK      0x0
#define ATT_BLUE       0x1
#define ATT_GREEN      0x2
#define ATT_CYAN       0x3
#define ATT_RED        0x4
#define ATT_MAGENTA    0x5
#define ATT_BROWN      0x6
#define ATT_LT_GREY    0x7
#define ATT_DK_GREY    0x8
#define ATT_LT_BLUE    0x9
#define ATT_LT_GREEN   0xA
#define ATT_LT_CYAN    0xB
#define ATT_LT_RED     0xC
#define ATT_LT_MAGENTA 0xD
#define ATT_LT_BROWN   0xE
#define ATT_WHITE      0xF

static int xpos = 0;
static int ypos = 0;

#include <multitasking.h>

void cls (void);
void itoa (char *buf, int base, int d);
void putchar (int c, TASK_LL *current_proc);
void printf(const char *format, ...);
void kprintf(const char *format, ...);
TASK_DISP_INFO create_task_disp(TASK_DISP_INFO *curr, int xmin, int xmax, int ymin, int ymax);

// #define putc(c) putchar(c, current_item)
#define FULL_DISPLAY(curr) create_task_disp(curr, 0, COLUMNS, 0, LINES);
#define LH_DISPLAY(curr) create_task_disp(curr, 0, COLUMNS/2, 0, LINES);
#define RH_DISPLAY(curr) create_task_disp(curr, COLUMNS/2, COLUMNS, 0, LINES);

typedef struct SCR_CHAR {
    char ch;
    char attribute;
}__attribute__((packed)) sc_char;

typedef struct SCREEN_O {
    sc_char chars[COLUMNS*LINES];
    struct SCREEN_O *next;
}__attribute__((packed)) screen;

static volatile screen *rel_video = (screen *)0x5000000;