#include <termio.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>

/* defines */
#define CTRL_KEY(k) ((k) & 0x1f)

/* data */
// stores the original state of terminal, so when program exits it should be returned to user in the same 
// state it was taken.
// we also need the width and height of the terminal, to print '~' 
struct editorConfig {
  int cx,cy; // keeping track of mouse position , cx- horizonal ,cy - vertical
  struct termios orig_termios;  
  int screenrows;
  int screencols;

};

struct editorConfig E;

/* append buffer : because C does not have dynamic strings, so we create our own for appending*/
struct abuf
{
  char *b;
  int len;
};

#define ABUF_INIT {NULL, 0} // empty buffer, acts as constructor

void abAppend(struct abuf *ab, const char *s, int len) {
  char *new = realloc(ab->b, ab->len + len); //making space for new string
  if (new == NULL) return;

  memcpy(&new[ab->len], s, len);
  ab->b = new;
  ab->len += len;
}

void abFree(struct abuf *ab) {
  free(ab->b);
}

/* terminal */

void die(const char *s) {
  write(STDOUT_FILENO, "\x1b[2J", 4); //cleans the terminal for new UI
  write(STDOUT_FILENO, "\x1b[H", 3); // reposition of cursor
  perror(s);
  exit(1);
}

void disableRawMode()
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
        die("tcsetattr");
    
}

void enableRawMode() 
{   
    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
    atexit(disableRawMode);

    struct termios raw = E.orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == 1)
        die("tcsetattr");
}

char editorKeyRead() {
    char c;
    int nread;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) die("read");
    }
    if (c == '\x1b') {
      char seq[3];
      if (read(STDIN_FILENO, &seq[0], 1)!= -1) return '\x1b';
      if (read(STDIN_FILENO, &seq[1], 1)!= -1) return '\x1b';
      
      if (seq[0] == '[') {
        switch (seq[1]) {
        case 'A': return 'w';
        case 'B': return 's';
        case 'C': return 'd';
        case 'D': return 'a';
        }
      } 

      return '\x1b';
      } else {
        return c;
      }
      }

int getWindowSize(int *rows, int *cols) {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    return -1;
  } else {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}

/* input */

void editorCursorMove(char keypress) {
  switch (keypress) {
  case 'w':
    E.cy--;
    break;
  case 'a':
    E.cx--;
    break;
  case 's':
    E.cy++;
    break;
  case 'd':
    E.cx++;
    break;
  }
}
// mapping keys to editor functions
void editorProcessKeypress() {
    char c = editorKeyRead();

    switch (c) {
    case CTRL_KEY('q'):
        write(STDOUT_FILENO, "\x1b[2J", 4); //cleans the terminal for new UI
        write(STDOUT_FILENO, "\x1b[H", 3); // reposition of cursor
        exit(0);
        break;
    
    case 'w':
    case 'a':
    case 's':
    case 'd':
      editorCursorMove(c);
      break;
    }
}

/* output */
void drawWelcomeMessage(struct abuf *ab) {
  char welcome[80];
  int welcomelen = snprintf(welcome, sizeof(welcome),
    "Kilo editor -- version 0.0.1");
  
  abAppend(ab, welcome, welcomelen);
  
}

void editorDrawRows(struct abuf *ab) {
  int y;
  for (y = 0; y < E.screenrows; y++) {
    if (y == E.screenrows / 3) {
      // Welcome message
      char welcome[80];
      int welcomelen = snprintf(welcome, sizeof(welcome),
        "Kilo editor -- version 0.0.1");

      if (welcomelen > E.screencols) welcomelen = E.screencols;
      // centering it
      int padding = (E.screencols - welcomelen) / 2;
      if (padding) {
        abAppend(ab, "~", 1);
        padding--;
      }
      while (padding--) abAppend(ab, " ", 1);

      abAppend(ab, welcome, welcomelen);
      }
      else {
        abAppend(ab, "~", 1);
      }
    
    abAppend(ab, "\x1b[K", 3);

    if (y < E.screenrows -1) {
        abAppend(ab, "\r\n", 2);
    }
    
  }
}

void editorRefreshScreen() {
    // \x1b: escape, always followed by [
    // 2 is a argument
    // 4 means 4 characters to write.
    struct abuf ab = ABUF_INIT;

    // with abAppend, no need to write bunch of write() 
    // abAppend(&ab, "\x1b[2J", 4);
    abAppend(&ab, "\x1b[H", 3);

    editorDrawRows(&ab);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy + 1, E.cx + 1); // specifying the position we want the cursor to move to 
    abAppend(&ab, buf, strlen(buf));


    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}

/** init ***/

void initEditor() {
    E.cx =0;
    E.cy=0;

    if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
}


int main(int argc, char const *argv[]) {
    enableRawMode();
    initEditor();

    while (1) {
        editorRefreshScreen();
        editorProcessKeypress();
    }
    // struct abuf ab = ABUF_INIT;
    // abAppend(&ab, "mohit", 5);
    // printf("new strings is %s\n", ab.b);
    // char c = 'w';
    // while (c != 'q')
    // {
    //   scanf("%c", &c);
    //   abAppend(&ab, &c, 1);
    //   printf("%s: %d\n",ab.b, ab.len);
    // }
    // free(ab.b); 
 
    return 0;
}
