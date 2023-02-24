#include <stdarg.h>
#include <vga.h>

// Array of screens defined

// typedef struct {
//     SCR_CHAR ch[COLUMNS];
// } TASK_BAR;
// void k_taskbar() {
//     char *bar = "1|2|3|4|5";
//     int xpos_a = 0;
//     int ypos_a = 50;

//     TASK_BAR bar = 0; 

//     int task_number = 0;
//     // Get number of processes, has it changed?
//     // Get current process, has it changed?

//     while (1) {        
//         // Refresh task list
//         for (int i=0; i<COLUMNS; i++) {
//             bar.ch = {0};
//             if ((TASK_LL *ret = TASK(i)) != NULL) {
//                 SCR_CHAR new_char = { 0 };
                
//                 bar.ch[ret->PID] = new_char;
//             }

//         }
        
//         // if (current_display != temp_curr_display) {
//         //     // Change colour
//         // }

//         // Draw screen
//         // vga putchar should take SCR_CHAR to print
        
//         char c = *bar;
//         int i=0;
//         for (char *temp = bar; *temp != '\0'; c = temp, temp++, i++) {
//             *((uint16_t *)video + (i + (LINES)*COLUMNS)) = *temp | ATT_LT_GREY << 12 | ATT_BLACK << 8;
//             // *((uint16_t *)video + (1 + 0)) = c | ATT_LT_GREY << 12 | ATT_BLACK << 8;
//         }

//         // }
//     }
// }

void cls (void) {
  xpos = 0; ypos = 0;
  int i;

  video = (unsigned char *) VIDEO;
  
  for (i = 0; i < COLUMNS * LINES * 2; i++)
    *(video + i) = 0xFFFF;

}



// TASK_DISP_INFO create_task_disp(TASK_DISP_INFO *curr, int xmin, int xmax, int ymin, int ymax) {
//     TASK_DISP_INFO ret; ret.xpos = xmin; ret.ypos = ymin; ret.xmin = xmin; ret.xmax = xmax; ret.ymin = ymin; ret.ymax = ymax;
//     ret.rel_video = (unsigned char *)HEAP_START;
    
//     // if (curr != NULL) {
//     //     // Can work on moving bytes over - not essential
//     // }

//     return ret;
// }

/*  Convert the integer D to a string and save the string in BUF. If
   BASE is equal to ’d’, interpret that D is decimal, and if BASE is
   equal to ’x’, interpret that D is hexadecimal. */
/*  Put the character C on the screen. */
void newline(TASK_DISP_INFO *display_blk) {
    display_blk->xpos = display_blk->xmin;
    display_blk->ypos++;
    if (display_blk->ypos >= display_blk->ymax)
        display_blk->ypos = display_blk->ymin;
    return;
}

void putchar_current(int c) {
    putchar(c, current_screen, screen_arr[current_screen].selected_cont);
}

void putchar_proc(int c, int pid) {
    for (int i=1; i<no_screens; i++) {
        SCREEN_O curr = screen_arr[i];
        for (int ii=0; ii<curr.cont_size; ii++) {
            CONTAINER cont = curr.conts[ii];
            if (cont.pid == pid) {
                putchar(c, i, ii);
                return;
            }
        }
    }
}

void putchar (int c, int screen_id, int cont_id) {
    // assert(screen_id <= MAX_SCREEN_NO);
    SCREEN_O *sel_screen = &(screen_arr[screen_id]);
    if (sel_screen->id == 0) return;
    // assert(cont_id <= MAX_CONTAINER_SIZE);

    CONTAINER *sel_container = &(sel_screen->conts[cont_id]);

    // if (sel_container == NULL) return;

    TASK_DISP_INFO *display_blk = &(sel_container->display_blk);

    if (c == '\n' || c == '\r')
    {
        newline(display_blk);
        return;
    }

    if (current_display == sel_screen->id) {
        (((SCREEN_O *)video)->chars)[(display_blk->xpos + display_blk->ypos * COLUMNS)].ch = (char)c;
        (((SCREEN_O *)video)->chars)[(display_blk->xpos + display_blk->ypos * COLUMNS)].attribute = ATT_BLACK << 4 | ATT_LT_GREY;
    } 

    (sel_screen->chars)[(display_blk->xpos + display_blk->ypos * COLUMNS)].ch = (char)c;
    (sel_screen->chars)[(display_blk->xpos + display_blk->ypos * COLUMNS)].attribute = ATT_BLACK << 4 | ATT_LT_GREY;


    display_blk->xpos++;
    if (display_blk->xpos >= display_blk->xmax)
        newline(display_blk);
}

void kputchar (int c) {
  if (c == '\n' || c == '\r')
    {
    knewline:
      xpos = 0;
      ypos++;
      if (ypos >= LINES)
        ypos = 0;
      return;
    }

    (((SCREEN_O *)video)->chars)[(xpos + ypos * COLUMNS)].ch = (char)c;
    (((SCREEN_O *)video)->chars)[(xpos + ypos * COLUMNS)].attribute = ATT_BLACK << 4 | ATT_RED;

  xpos++;
  if (xpos >= COLUMNS)
    goto knewline;
}

char *_itoa(int num, int base, char *buffer) {
    char rep[16] = "0123456789ABCDEF";
    char *ptr = &buffer[49];
    *ptr = '\0';

    do {
        *--ptr = rep[num%base];
        num /= base;
    } while( num != 0 );

    return ptr;
}

int is_digit(char let) {
    return let >= 48 && let <= 57; 
}

int _atoi(char let) {
    if (let >= 48 && let <= 57) return let-48;
    else return -1;
}

int str_len(char *string) {
    int ret = 0;
    for (; *string != '\0'; string++)
        ret++;
    return ret;
}

void printf(const char *format, ...) {

    va_list arg;
    va_start(arg, format);

    char *string;

    for (string=format; *string != '\0'; string++) {
        if ( *string == '%' ) {
            string++;

            int padding = 0;
            for (; is_digit(*string); string++) {
                if (padding != 0)
                    padding *= 10;
                padding += _atoi(*string);
            }

            char retstr[50];
            int dec = 0;
            int base = 0;
            char *str;

            char paddingChar;
            if ( *string == 'b' || *string == 'd' || *string == 'o' || *string == 'x' || *string == 'c' ) {
                paddingChar = '0';
                dec = va_arg(arg, int);
            } 

            switch ( *string ) {
                case 'c':
                    putchar_proc(dec, current_item->PID);
                    string++;
                    continue;
                    break;
                case 'd':
                    base = 10;
                    break;
                case 'o':
                    base = 8;
                    break;
                case 'b':
                    base = 2;
                    break;
                case 's':
                    paddingChar = ' ';
                    str = va_arg(arg, char *);
                    break;
                case 'x':
                    base = 16;
                    break;
                default:
                    putchar_proc(*string, current_item->PID);
            }

            if (base != 0) {
                str = _itoa(dec, base, retstr);
            }

            for (; padding > str_len(str); padding--)
                putchar_proc(paddingChar, current_item->PID);

            for (; *str != '\0'; str++)
                putchar_proc(*str, current_item->PID);

            string++;
        }

        putchar_proc(*string, current_item->PID);
    }

    return;
}

void kprintf(const char *format, ...) {

    va_list arg;
    va_start(arg, format);

    char *string;

    for (string=format; *string != '\0'; string++) {
        if ( *string == '%' ) {
            string++;

            int padding = 0;
            for (; is_digit(*string); string++) {
                if (padding != 0)
                    padding *= 10;
                padding += _atoi(*string);
            }

            char retstr[50];
            int dec = 0;
            int base = 0;
            char *str;

            char paddingChar;
            if ( *string == 'b' || *string == 'd' || *string == 'o' || *string == 'x' || *string == 'c' ) {
                paddingChar = '0';
                dec = va_arg(arg, int);
            } 

            switch ( *string ) {
                case 'c':
                    kputchar(dec);
                    string++;
                    continue;
                    break;
                case 'd':
                    base = 10;
                    break;
                case 'o':
                    base = 8;
                    break;
                case 'b':
                    base = 2;
                    break;
                case 's':
                    paddingChar = ' ';
                    str = va_arg(arg, char *);
                    break;
                case 'x':
                    base = 16;
                    break;
                default:
                    kputchar(*string);
            }

            if (base != 0) {
                str = _itoa(dec, base, retstr);
            }

            for (; padding > str_len(str); padding--)
                kputchar(paddingChar);

            for (; *str != '\0'; str++)
                kputchar(*str);

            string++;
        }

        kputchar(*string);
    }

    return;
}