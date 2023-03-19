#include <stdarg.h>
#include <vga.h>


static volatile uint16_t *video = (uint16_t *)VIDEO;
static int xpos = 0;
static int ypos = 0;


/*  Convert the integer D to a string and save the string in BUF. If
   BASE is equal to ’d’, interpret that D is decimal, and if BASE is
   equal to ’x’, interpret that D is hexadecimal. */
/*  Put the character C on the screen. */
void newline(TASK_DISP_INFO *display_blk) {
    display_blk->xpos = display_blk->xmin;
    display_blk->ypos++;
    if (display_blk->ypos > display_blk->ymax)
        display_blk->ypos = display_blk->ymin;

    return;
}

SCR_CHAR to_char_mod(int c, int ATTRIBUTES) {
    SCR_CHAR ret;
    ret.ch = c;
    ret.attribute = ATTRIBUTES;
    return ret;
}


void putchar_current(int c) {
    putchar(DEFAULT_CHAR(c), current_screen->id, current_screen->selected_cont);
}

// Writes to every buffer containing process
void putchar_variable(SCR_CHAR char_mod, int pid) {
    for (SCREEN_O *temp = screen_root; temp != NULL; temp = temp->next) {
        for (int ii=0; ii<temp->cont_size; ii++) {
            CONTAINER cont = temp->conts[ii];
            if (cont.pid == pid) {
                putchar(char_mod, temp->id, ii);
            }
        }
    }
}

extern CONTAINER *find_container(int pid);
void cls (void) {
    SCR_CHAR c = CLEAR_CHAR();
    
    for (int i = 0; i < COLUMNS * LINES; i++)
        putchar_variable(c, current_item->PID);

    CONTAINER *cont = find_container(current_item->PID);
    cont->display_blk.xpos = 0;
    cont->display_blk.ypos = 0;
    if (cont->pid == current_screen->conts[current_screen->selected_cont].pid) move_cursor(cont->display_blk.xpos, cont->display_blk.ypos);

}



void putchar_proc(int c, int pid) {
    for (SCREEN_O *temp = screen_root; temp != NULL; temp = temp->next) {
        for (int ii=0; ii<temp->cont_size; ii++) {
            CONTAINER cont = temp->conts[ii];
            if (cont.pid == pid) {
                putchar(DEFAULT_CHAR(c), temp->id, ii);
                return; // Not needed
            }
        }
    }
}



extern SCREEN_O * find_screen(int id);
void putchar (SCR_CHAR char_mod, int screen_id, int cont_id) {
    // assert(screen_id <= MAX_SCREEN_NO);
    SCREEN_O *sel_screen = find_screen(screen_id);
    if (sel_screen == NULL || sel_screen->id == 0) return;
    // assert(cont_id <= MAX_CONTAINER_SIZE);

    CONTAINER *sel_container = &(sel_screen->conts[cont_id]);

    // if (sel_container == NULL) return;

    TASK_DISP_INFO *display_blk = &(sel_container->display_blk);

    char c = char_mod.ch;

    if (c == '\n' || c == '\r')
    {
        newline(display_blk);
        return;
    }

    if (current_screen->id == sel_screen->id) {
        (((SCREEN_O *)video)->chars)[(display_blk->xpos + display_blk->ypos * COLUMNS)] = char_mod;
        if (cont_id == sel_screen->selected_cont) move_cursor(display_blk->xpos, display_blk->ypos);
    } 

    (sel_screen->chars)[(display_blk->xpos + display_blk->ypos * COLUMNS)] = char_mod;


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

int atoi(char *str) {
    int number = 0;
    int i = 0;

    // Iterate through each character in the string
    while (str[i] != '\0') {
        // Convert the character to its numeric value and add it to the number
        number = number * 10 + (str[i] - '0');
        i++;
    }

    return number;
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

    const char *string;

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
                case '?':
                    SCR_CHAR *str_mod = va_arg(arg, SCR_CHAR *);
                    for (SCR_CHAR *temp=str_mod; temp->ch != '\0'; temp++) {
                        putchar_variable(*temp, current_item->PID);
                    }
                    string++;
                    continue;
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

    const char *string;

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