#include <stdarg.h>
#include <vga.h>

void cls (void) {
  xpos = 0; ypos = 0;
  int i;

  video = (unsigned char *) VIDEO;
  
  for (i = 0; i < COLUMNS * LINES * 2; i++)
    *(video + i) = 0xFFFF;

}

TASK_DISP_INFO create_task_disp(TASK_DISP_INFO *curr, int xmin, int xmax, int ymin, int ymax) {
    TASK_DISP_INFO ret; ret.xpos = xmin; ret.ypos = ymin; ret.xmin = xmin; ret.xmax = xmax; ret.ymin = ymin; ret.ymax = ymax;
    ret.rel_video = (unsigned char *)HEAP_START;
    
    // if (curr != NULL) {
    //     // Can work on moving bytes over - not essential
    // }

    return ret;
}

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
// The problem with this is current proc is a pointer to heap space
// Interrupts can be called while this is here, changing current process running
// That may be why it's fricking up. To prove this I will change PIT frequency and the error will be less extreme.
void putchar (int c, TASK_LL *current_proc) {

  TASK_DISP_INFO *display_blk = &(current_proc->display_blk);
  if (c == '\n' || c == '\r')
    {
      newline(display_blk);
      return;
    }

  // Need to be able to get current process information from shared process page
  if (current_proc->PID == current_display->PID) {
    // *(((unsigned char *)HEAP_START) + (display_blk->xpos + display_blk->ypos * COLUMNS) * 2) = c & 0xFF;
    // *(((unsigned char *)HEAP_START) + (display_blk->xpos + display_blk->ypos * COLUMNS) * 2 + 1) = ATT_LT_GREY;

    (((screen *)video)->chars)[(display_blk->xpos + display_blk->ypos * COLUMNS)].ch = (char)c;
    (((screen *)video)->chars)[(display_blk->xpos + display_blk->ypos * COLUMNS)].attribute = ATT_BLACK << 4 | ATT_LT_GREY;
    // *((uint16_t *)(((unsigned char *)video) + (display_blk->xpos + display_blk->ypos * COLUMNS) * 2)) = *((uint16_t *)(((unsigned char *)HEAP_START) + (display_blk->xpos + display_blk->ypos * COLUMNS) * 2));
    // *(((unsigned char *)video) + (display_blk->xpos + display_blk->ypos * COLUMNS) * 2) = c & 0xFF;
    // *(((unsigned char *)video) + (display_blk->xpos + display_blk->ypos * COLUMNS) * 2 + 1) = ATT_LT_GREY;
    // *(video + (display_blk->xpos + display_blk->ypos)) = c & 0xFF;
    // *(video + (display_blk->xpos + display_blk->ypos) + 1) = ATT_LT_GREY;
    // *((uint16_t *)video + (display_blk->xpos + display_blk->ypos)) = c | ATT_LT_GREY << 12 | ATT_BLACK << 8;
  } 
    (((screen *)rel_video)->chars)[(display_blk->xpos + display_blk->ypos * COLUMNS)].ch = (char)c;
    (((screen *)rel_video)->chars)[(display_blk->xpos + display_blk->ypos * COLUMNS)].attribute = ATT_BLACK << 4 | ATT_LT_GREY;
  

  // to check
//   *(video + (xpos + ypos * COLUMNS) * 2) = (c | ATT_BLACK << 12 | ATT_LT_GREY << 8);

  // Update 0xb8000 if needed 

  display_blk->xpos++;
  if (display_blk->xpos > display_blk->xmax)
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

    (((screen *)video)->chars)[(xpos + ypos * COLUMNS)].ch = (char)c;
    (((screen *)video)->chars)[(xpos + ypos * COLUMNS)].attribute = ATT_BLACK << 4 | ATT_RED;

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
                    putchar(dec, (TASK_LL *)PROCESS_CONT_ADDR);
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
                    putchar(*string, (TASK_LL *)PROCESS_CONT_ADDR);
            }

            if (base != 0) {
                str = _itoa(dec, base, retstr);
            }

            for (; padding > str_len(str); padding--)
                putchar(paddingChar, (TASK_LL *)PROCESS_CONT_ADDR);

            for (; *str != '\0'; str++)
                putchar(*str, (TASK_LL *)PROCESS_CONT_ADDR);

            string++;
        }

        putchar(*string, (TASK_LL *)PROCESS_CONT_ADDR);
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