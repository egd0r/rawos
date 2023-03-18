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

int HALF_DISPLAY(int pid, int sid) {
    return new_disp(sid, pid, 0, COLUMNS/2, 0, LINES-1);
}

TASK_DISP_INFO NEW_FULL_DISPLAY() {
    TASK_DISP_INFO ret; ret.xpos = 0; ret.ypos = 0; ret.xmin = 0; ret.xmax = COLUMNS; ret.ymin = 0; ret.ymax = LINES-1;
    return ret;
}

TASK_DISP_INFO NEW_RH_DISPLAY() {
    TASK_DISP_INFO ret; ret.ypos = 0; ret.xmin = COLUMNS/2 + 1; ret.xpos = ret.xmin; ret.xmax = COLUMNS; ret.ymin = 0; ret.ymax = LINES-1;
    return ret;
}

TASK_DISP_INFO NEW_LH_DISPLAY() {
    TASK_DISP_INFO ret; ret.ypos = 0; ret.xmin = 0; ret.xpos = ret.xmin; ret.xmax = COLUMNS/2; ret.ymin = 0; ret.ymax = LINES-1;
    return ret;
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
    if (current_screen == NULL) {
        current_screen = screen;
        screen_root = screen;
        return 1;
    }
    
    screen->next = NULL;
    SCREEN_O *temp;
    for (temp = screen_root; temp->next != NULL; temp=temp->next);
    temp->next = screen;
    return 1;
}

CONTAINER taskbar_cont = {0};
int taskbar_disp(int pid) {
    CONTAINER cont;
    memset((uint64_t)&cont, 0, sizeof(cont));


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

    taskbar_cont = cont;
    return 1;
}

// Array of screens defined

// typedef struct {
//     SCR_CHAR ch[COLUMNS];
// } TASK_BAR;
void sys_printf(const char *format, ...);
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
    if (current_screen->id == new_screen_id) return -1;
    if (new_screen_id > no_screens || new_screen_id == 0) return -1;

    SCREEN_O *to_swap = find_screen(new_screen_id);
    if (to_swap == NULL) return -1;

    SCR_CHAR *buffer = (SCR_CHAR *)(&(to_swap->chars));
    memcpy((uint8_t *)buffer, (uint8_t *)VIDEO_ACTUAL, COLUMNS * LINES * 2);

    current_screen = to_swap;

    return 1;
}

void map_screen(SCREEN_O *scr, TASK_DISP_INFO bounds) {
    struct CONT_O existing_cont = scr->conts[scr->selected_cont];
    TASK_DISP_INFO existing_disp = existing_cont.display_blk;
    // Map bounds of existing display block to LHS
    // Starting at end, work back until first character is found in screen
    SCR_CHAR *chars = scr->chars;
    int curr_x = bounds.xmax;
    int curr_y = bounds.ymax;
    for (int i=(existing_disp.xmax+existing_disp.ymax * COLUMNS); i>0; i--) {
        if (chars[i].ch == 0xFF || chars[i].ch == 0x00) continue;
        // If contains char begin mapping
        int temp_index = (curr_x + curr_y * COLUMNS);
        chars[temp_index] = chars[i];
        chars[i] = CLEAR_CHAR();

        curr_x--;
        if (curr_x < 0) {
            curr_y--;
            curr_x = bounds.xmax;
        }
    }
}

// Returns ID of new screen created
int new_disp(int curr, int pid, int xmin, int xmax, int ymin, int ymax) {
    if (curr == -1) return -1; // Proc wants to output to 0

    TASK_DISP_INFO ret;

    SCREEN_O *new_scr = NULL;

    CONTAINER cont;
    memset((uint64_t)&cont, 0, sizeof(cont));
    cont.stream.position = -1;

    if (curr != 0)
        new_scr = find_screen(curr);

    int make_tb = 0;
    if (new_scr == NULL) {
        new_scr = (SCREEN_O *)kp_alloc(2);
        memset((uint64_t)new_scr, 0, sizeof(SCREEN_O));
        ret = NEW_FULL_DISPLAY();
        make_tb = 1;
    } else {
        // ret is new display block
        ret = NEW_RH_DISPLAY();
        // need to map existing container to left side
        TASK_DISP_INFO new_existing = NEW_LH_DISPLAY();
        // Map current buffer to new bounds
        map_screen(new_scr, new_existing);
    }

    cont.parent_screen_id = no_screens;
    new_scr->id = no_screens;
    no_screens++;
    cont.display_blk = ret;
    cont.pid = pid;

    new_scr->conts[(new_scr->cont_size)++] = cont;
    if (make_tb) new_scr->conts[(new_scr->cont_size)++] = taskbar_cont;

    add_screen(new_scr);
    return no_screens-1;
}

