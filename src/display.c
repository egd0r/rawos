#include <display.h>
#include <memory.h>

CONTAINER *current_container = NULL;

SCREEN_O *screen_root = NULL;

int no_screens = 1; // Always starts at 1 to allow for /dev/null
SCREEN_O screen_arr[COLUMNS] = { 0 };


SCREEN_O * current_screen = NULL;

int FULL_DISPLAY(int pid) {
    return new_disp(0, pid, 0, COLUMNS, 0, LINES-1);
}

CONTAINER *find_container(int pid) {
    for (SCREEN_O *temp = screen_root; temp != NULL; temp=temp->next) {
        for (int i=0; i<temp->cont_size; i++) {
            CONTAINER cont = temp->conts[i];
            if (cont.pid == pid) return &(temp->conts[i]);
        }
    }

    return NULL;
}

void getch(char *buffer) {
	IN_STREAM *stream = &(find_container(current_item->PID)->stream);
	if (stream->position == -1) {
		buffer[0] = -1;
	} else {
		int ret = stream->buffer[(stream->position)--];
		buffer[0] = ret;
	} 
}

SCREEN_O * find_screen(int id) {
    for (SCREEN_O *temp = screen_root; temp != NULL; temp=temp->next) {
        if (temp->id == id) return temp;
    }
    return NULL;
}

int add_screen(SCREEN_O *screen) {
    SCREEN_O *temp;
    for (temp = screen_root; temp->next != NULL; temp=temp->next);
    temp->next = screen;
    return 1;
}

int taskbar_disp(int pid) {
    CONTAINER cont;
    memset(&cont, 0, sizeof(cont));


    TASK_DISP_INFO ret; ret.xpos = 0; ret.ypos = LINES; ret.xmin = 0; ret.xmax = COLUMNS; ret.ymin = LINES; ret.ymax = LINES;

    cont.pid = pid;
    cont.display_blk = ret;

    // for (int i=0; i<no_screens; i++) {
    //     cont.parent_screen_id = i;
    //     screen_arr[i].conts[screen_arr[i].cont_size++] = cont; // Adding taskbar container to each screen
    // }
// for dynamic allocation
    for (SCREEN_O *temp = screen_root; temp != NULL; temp=temp->next) {
        temp->conts[temp->cont_size] = cont;
        temp->cont_size++;
    }

    return 1;
}

// Array of screens defined

// typedef struct {
//     SCR_CHAR ch[COLUMNS];
// } TASK_BAR;
void k_taskbar() {
    SCR_CHAR *task_b = (SCR_CHAR *)new_malloc(sizeof(SCR_CHAR)*MAX_SCREEN_NO);

    while (1) {  
        int index=0;
        for (int i=1; index<COLUMNS; i++, index+=2) {
            task_b[index].ch = i<no_screens ? find_screen(i)->id + 0x30 : ' ';
            task_b[index].attribute = ATT_LT_GREY << 4 | (current_screen->id == i ? ATT_LT_RED : ATT_BLACK);
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

    SCREEN_O *to_swap = find_screen(new_screen_id);

    SCR_CHAR *buffer = &(to_swap->chars);
    memcpy(buffer, VIDEO_ACTUAL, COLUMNS * LINES * 2);

    current_screen = to_swap;
}

// Returns ID of new screen created
int new_disp(int curr, int pid, int xmin, int xmax, int ymin, int ymax) {
    if (curr == -1) return 0; // Proc wants to output to 0

    TASK_DISP_INFO ret; ret.xpos = xmin; ret.ypos = ymin; ret.xmin = xmin; ret.xmax = xmax; ret.ymin = ymin; ret.ymax = ymax;

    SCREEN_O *new_scr;

    CONTAINER cont;
    memset(&cont, 0, sizeof(cont));
    cont.stream.position = -1;

    if (curr != 0)
        new_scr = find_screen(curr);

    new_scr = (SCREEN_O *)kp_alloc(2);
    memset(new_scr, 0, sizeof(SCREEN_O));

    cont.display_blk = ret;
    cont.pid = pid;
    cont.parent_screen_id = no_screens;

    new_scr->id = pid;
    new_scr->conts[(new_scr->cont_size)++] = cont;

    if (current_screen == NULL) {
        current_screen = new_scr;
        screen_root = new_scr;
    } else {
        add_screen(new_scr);
    }

    no_screens++;

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