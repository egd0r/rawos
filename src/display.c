#include <display.h>

CONTAINER *current_container = NULL;

CONTAINER *ready_start_container = NULL;
CONTAINER *ready_end_container = NULL;

int no_screens = 1; // Always starts at 1 to allow for /dev/null
SCREEN_O screen_arr[COLUMNS] = { 0 };


int current_screen = 1;

int FULL_DISPLAY(int pid) {
    return new_disp(0, pid, 0, COLUMNS, 0, LINES-1);
}

CONTAINER *find_container(int pid) {
 for (int i=1; i<no_screens; i++) {
        SCREEN_O curr = screen_arr[i];
        for (int ii=0; ii<curr.cont_size; ii++) {
            CONTAINER cont = curr.conts[ii];
            if (cont.pid == pid) {
                return &(screen_arr[i].conts[ii]); // Not needed
            }
        }
    }

    return NULL;
}

// void getch(char *buffer) {

// 	IN_STREAM stream;
// 	if (stream.position == -1) {
// 		return -1;
// 	} else {
//         CONTAINER *proc_cont = find_container(current_item->PID);
//         if (proc_cont == NULL) return;

//         // IN_STREAM stream = proc_cont->stream;

// 		// buffer[0] = stream.buffer[(stream.position)--];
// 	} 
// }

int taskbar_disp(int pid) {
    CONTAINER cont;
    memset(&cont, 0, sizeof(cont));


    TASK_DISP_INFO ret; ret.xpos = 0; ret.ypos = LINES; ret.xmin = 0; ret.xmax = COLUMNS; ret.ymin = LINES; ret.ymax = LINES;

    cont.pid = pid;
    cont.display_blk = ret;

    for (int i=0; i<no_screens; i++) {
        cont.parent_screen_id = i;
        screen_arr[i].conts[screen_arr[i].cont_size++] = cont; // Adding taskbar container to each screen
    }

}

// Array of screens defined

// typedef struct {
//     SCR_CHAR ch[COLUMNS];
// } TASK_BAR;
void k_taskbar() {
    char *bar = "1|2|3|4|5";

    SCR_CHAR *task_b = (SCR_CHAR *)new_malloc(sizeof(SCR_CHAR)*MAX_SCREEN_NO);
    // task_b[0].ch = 'E';
    // task_b[0].attribute = ATT_LT_BLUE << 4 | ATT_LT_MAGENTA;


    while (1) {  
        int index=0;
        for (int i=1; index<COLUMNS; i++, index+=2) {
            task_b[index].ch = i<no_screens ? screen_arr[i].id + 0x30 : ' ';
            task_b[index].attribute = ATT_LT_GREY << 4 | (current_screen == i ? ATT_LT_RED : ATT_BLACK);
            task_b[index+1].ch = i<no_screens ? '|' : ' ';
            task_b[index+1].attribute = ATT_LT_GREY << 4 | ATT_BLACK;
        }
        task_b[index].ch = '\0';      
        sys_printf("%?", task_b);
        
        // printf(bar);
    }
}

int swap_screens(int new_screen_id) {
    if (current_screen == new_screen_id) return -1;
    if (new_screen_id > no_screens || new_screen_id == 0) return -1;

    SCR_CHAR *buffer = &(screen_arr[new_screen_id].chars);
    memcpy(buffer, VIDEO_ACTUAL, COLUMNS * LINES * 2);

    current_screen = new_screen_id;
}

// Returns ID of new screen created
int new_disp(int curr, int pid, int xmin, int xmax, int ymin, int ymax) {
    if (curr == -1) return 0; // Proc wants to output to 0

    TASK_DISP_INFO ret; ret.xpos = xmin; ret.ypos = ymin; ret.xmin = xmin; ret.xmax = xmax; ret.ymin = ymin; ret.ymax = ymax;

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