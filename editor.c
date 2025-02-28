#include <termio.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

/* defines */
#define CTRL_KEY(k) ((k) & 0x1f)

/* data */
// stores the original state of terminal, so when program exits it should be returned to user in the same 
// state it was taken.
struct termios orig_termios;  

/* terminal */

void die(const char *s) {
  write(STDOUT_FILENO, "\x1b[2J", 4); //cleans the terminal for new UI
  write(STDOUT_FILENO, "\x1b[H", 3); // reposition of cursor
  perror(s);
  exit(1);
}

void disableRawMode()
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
        die("tcsetattr");
    
}

void enableRawMode() 
{   
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) die("tcgetattr");
    atexit(disableRawMode);

    struct termios raw = orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == 1)
        die("tcsetattr");
}

char editorKeyRead()
{
    char c;
    int nread = read(STDIN_FILENO, &c, 1);
    while (nread != 1) {
        if (nread == -1 && errno != EAGAIN) die("read");
        printf("%c ",c);
    }
    return c;
}

/* input */

// mapping keys to editor functions
void editorProcessKeypress() {
    char c = editorKeyRead();

    switch (c) {
    case CTRL_KEY('q'):
        write(STDOUT_FILENO, "\x1b[2J", 4); //cleans the terminal for new UI
        write(STDOUT_FILENO, "\x1b[H", 3); // reposition of cursor
        exit(0);
        break;
    }
}

void editorRefreshScreen() {
    // \x1b: escape, always followed by [
    // 2 is a argument
    // 4 means 4 characters to write.
    write(STDOUT_FILENO, "\x1b[2J", 4); //cleans the terminal for new UI
    write(STDOUT_FILENO, "\x1b[H", 3); // reposition of cursor
}

/** init ***/
int main(int argc, char const *argv[])
{
    enableRawMode();

    while (1) {
        editorRefreshScreen();
        editorProcessKeypress();
    }
     
    
    return 0;
}
