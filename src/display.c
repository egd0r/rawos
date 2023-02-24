#include <display.h>

CONTAINER *current_container = NULL;

CONTAINER *ready_start_container = NULL;
CONTAINER *ready_end_container = NULL;

int no_screens = 1; // Always starts at 1 to allow for /dev/null
SCREEN_O screen_arr[COLUMNS] = { 0 };


int current_screen = 1;

// Returns ID of new screen created
int new_disp(int curr, int pid, int xmin, int xmax, int ymin, int ymax) {
    if (curr == -1) return 0; // Proc wants to output to 0

    TASK_DISP_INFO ret; ret.xpos = xmin; ret.ypos = ymin; ret.xmin = xmin; ret.xmax = xmax; ret.ymin = ymin; ret.ymax = ymax;
    ret.rel_video = (unsigned char *)HEAP_START;

    CONTAINER cont;
    memset(&cont, 0, sizeof(cont));
    SCREEN_O new_screen;
    memset(&new_screen, 0, sizeof(new_screen));

    if (curr != 0)
        new_screen = screen_arr[curr];

    cont.display_blk = ret;
    cont.pid = pid;
    cont.parent_screen_id = no_screens;

    new_screen.id = pid;
    new_screen.conts[(new_screen.cont_size)++] = cont;

    screen_arr[no_screens++] = new_screen;

    return no_screens-1;
}

CONTAINER *remove_screen () {
    // TODO...
}

void attach_proc_to_screen(TASK_LL *proc, int container_id) {
    // Add to proc list of container
}

void remove_proc_from_screen(TASK_LL *proc, int container_id) {

}