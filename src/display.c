#include <display.h>
#include <memory.h>

SCREEN_O *screen_root = NULL;
SCREEN_O * current_screen = NULL;

int no_screens = 1; // Always starts at 1 to allow for /dev/null

TASK_DISP_INFO NEW_FULL_DISPLAY() {
    TASK_DISP_INFO ret; ret.xpos = 0; ret.ypos = 0; ret.xmin = 0; ret.xmax = COLUMNS; ret.ymin = 0; ret.ymax = LINES-1;
    return ret;
}

TASK_DISP_INFO NEW_RH_DISPLAY() {
    TASK_DISP_INFO ret; ret.ypos = 0; ret.xmin = COLUMNS/2; ret.xpos = ret.xmin; ret.xmax = COLUMNS; ret.ymin = 0; ret.ymax = LINES-1;
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
    if (id == -1 || id == 0) return NULL;
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
            SCREEN_O *scr = find_screen(i);
            if (scr == NULL && i<no_screens) {
                index-=2;
                continue; 
            }
            task_b[index].ch = i<no_screens ? scr->id + 0x30 : ' ';
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
    // if (current_screen->id == new_screen_id) return -1;
    if (new_screen_id > no_screens || new_screen_id == 0) return -1;

    SCREEN_O *to_swap = find_screen(new_screen_id);
    if (to_swap == NULL) return -1;

    SCR_CHAR *buffer = (SCR_CHAR *)(&(to_swap->chars));
    memcpy((uint8_t *)buffer, (uint8_t *)VIDEO_ACTUAL, COLUMNS * LINES * 2);

    current_screen = to_swap;

    return 1;
}

// Build a string out of every line and re-print it
void map_screen(SCREEN_O *scr, TASK_DISP_INFO *bounds) {
    struct CONT_O existing_cont = scr->conts[scr->selected_cont];
    TASK_DISP_INFO existing_disp = existing_cont.display_blk;
    SCR_CHAR *chars = scr->chars;

    // Create temporary buffer
    SCR_CHAR temp_chars[COLUMNS * (LINES + 1)] = {0};
    // memset(temp_chars, 0, sizeof(temp_chars));

    // Map existing screen buffer to left side of temp buffer
    int curr_x = bounds->xmin;
    int curr_y = bounds->ymin;
    // Starting half-way
    int existing_x = existing_disp.xmax / 2;
    int existing_y = existing_disp.ymax / 2;
    int new = 0;
    for (; (existing_x < existing_disp.xmax || existing_y < existing_disp.ymax) && (curr_x < bounds->xmax || curr_y < bounds->ymax); ) {
        int temp_index = curr_x + curr_y * COLUMNS;
        int i = existing_x + existing_y * COLUMNS;

        if (existing_x >= bounds->xmax && new == 0) {
            curr_y++;
            curr_x = bounds->xmin;
            new = 1;
        }

        temp_index = curr_x + curr_y * COLUMNS;
        i = existing_x + existing_y * COLUMNS;


        if (curr_x < bounds->xmax && curr_y < bounds->ymax)
            temp_chars[temp_index] = chars[i];

        if (existing_x == existing_disp.xpos && existing_y == existing_disp.ypos) {
            bounds->xpos = curr_x;
            bounds->ypos = curr_y;
        }

        curr_x++;
        if (curr_x > bounds->xmax) {
            curr_y++;
            curr_x = bounds->xmin;
        }

        existing_x++;
        if (existing_x > existing_disp.xmax) {
            existing_y++;
            existing_x = existing_disp.xmin;
            new = 0;
        }
    }

    SCR_CHAR c = CLEAR_CHAR();
    for (int i = 0; i < COLUMNS * LINES; i++) {
        chars[i] = c;
    }

    memcpy((uint8_t *)temp_chars, (uint8_t *)chars, COLUMNS * (LINES + 1));

    if (current_screen->id == scr->id) {
        move_cursor(bounds->xpos, bounds->ypos);
        swap_screens(scr->id);
    }
}


// Returns ID of new screen created
int new_disp(int sid, int pid) {
    if (sid == 0) return -1; // Proc wants to output to 0

    int curr_id = no_screens;
    TASK_DISP_INFO ret;

    SCREEN_O *new_scr = NULL;

    CONTAINER cont;
    memset((uint64_t)&cont, 0, sizeof(cont));
    cont.stream.position = -1;

    new_scr = find_screen(sid);

    int make_tb = 0;
    if (new_scr == NULL) {
        new_scr = (SCREEN_O *)kp_alloc(2);
        memset((uint64_t)new_scr, 0, sizeof(SCREEN_O));
        ret = NEW_FULL_DISPLAY();
        make_tb = 1;
        cont.parent_screen_id = no_screens;
        new_scr->id = no_screens;
        no_screens++;
    } else {
        // ret is new display block
        ret = NEW_RH_DISPLAY();
        // need to map existing container to left side
        TASK_DISP_INFO new_existing = NEW_LH_DISPLAY();
        // Map current buffer to new bounds
        map_screen(new_scr, &new_existing);
        new_scr->conts[0].display_blk = new_existing;
        curr_id = new_scr->id;
    }
    cont.display_blk = ret;
    cont.pid = pid;


    new_scr->conts[(new_scr->cont_size)++] = cont;
    if (make_tb) {
        new_scr->conts[(new_scr->cont_size)++] = taskbar_cont;
        add_screen(new_scr);
    }

    return curr_id;
}

void remove_display(int sid) {
    if (current_screen->id == sid) return;
    SCREEN_O *prev = NULL;
    for (SCREEN_O *temp = screen_root; temp != NULL && temp->id != sid; temp=temp->next) {
        prev = temp;
    }


    if (prev == NULL || prev->next == NULL) screen_root = screen_root->next;
    else {
        prev->next = prev->next->next;
    }
}

