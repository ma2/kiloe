#ifndef KILOE_H
#define KILOE_H

/** includes */

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

/** defines */

#define KILO_VERSION "0.0.1"
#define KILO_TAB_STOP (Config.tab_stop)
#define KILO_QUIT_TIMES (Config.quit_times)

#define CTRL_KEY(k) ((k) & 0x1f)
#define ESC '\x1b'
#define SPC ' '

#define ABUF_INIT {NULL, 0}
#define HLDB_ENTRIES (getHLDBEntries())

enum editorKey {
  BACKSPACE = 127,
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN,
  DELETE,
  HOME,
  END,
  PAGE_UP,
  PAGE_DOWN
};

enum editorHighLight {
  HL_NORMAL = 0,
  HL_COMMENT,
  HL_MLCOMMENT,
  HL_KEYWORD1,
  HL_KEYWORD2,
  HL_STRING,
  HL_NUMBER,
  HL_MATCH
};

#define HL_HIGHLIGHT_NUMBERS (1<<0)
#define HL_HIGHLIGHT_STRINGS (1<<1)

/** data structures */

struct editorSyntax {
  char *filetype;
  char **filematch;
  char **keywords;
  char *singleline_comment_start;
  char *multiline_comment_start;
  char *multiline_comment_end;
  int flags;
};

typedef struct erow {
  int idx;
  int size;
  int rsize;
  char *chars;
  char *render;
  unsigned char *hl;
  int hl_open_comment;
} erow;

struct editorConfig {
  int cx, cy;
  int rx;
  int rowoff;
  int coloff;
  int screenrows;
  int screencols;
  int numrows;
  erow *row;
  int dirty;
  char *filename;
  char statusmsg[80];
  time_t statusmsg_time;
  struct editorSyntax *syntax;
  struct termios orig_termios;
};

struct editorSettings {
  int tab_stop;
  int quit_times;
  int show_line_numbers;
  char welcome_message[256];
  int status_timeout;
  int color_comment;
  int color_keyword1;
  int color_keyword2;
  int color_string;
  int color_number;
  int color_match;
};

struct abuf {
  char *b;
  int len;
};

/** global variables */

extern struct editorConfig E;
extern struct editorSettings Config;
extern struct editorSyntax HLDB[];

/** terminal functions */

void die(const char *s);
void disableRawMode();
void enableRawMode();
int editorReadKey();
int getCursorPosition(int *rows, int *cols);
int getWindowSize(int *rows, int *cols);

/** utf8 functions */

int is_utf8_continuation(unsigned char c);
int move_to_next_char(char *str, int pos, int max);
int move_to_prev_char(char *str, int pos);
int get_char_width(char *str, int pos);

/** config functions */

void initDefaultConfig();
int loadConfig(const char *filename);

/** syntax highlighting functions */

int is_separator(int c);
void editorUpdateSyntax(erow *row);
int editorSyntaxToColor(int hl);
void editorSelectSyntaxHighlight();

/** row operations */

int editorRowCxToRx(erow *row, int cx);
int editorRowRxToCx(erow *row, int rx);
void editorUpdateRow(erow *row);
void editorInsertRow(int at, char *s, size_t len);
void editorFreeRow(erow *row);
void editorDelRow(int at);
void editorRowInsertChar(erow *row, int at, int c);
void editorRowAppendString(erow *row, char *s, size_t len);
void editorRowDelChar(erow *row, int at);

/** editor operations */

void editorInsertChar(int c);
void editorInsertNewLine();
void editorDelChar();

/** file io */

char *editorRowsToString(int *buflen);
void editorOpen(char *filename);
void editorSave();

/** find */

void editorFindCallback(char *query, int key);
void editorFind();

/** append buffer */

void abAppend(struct abuf *ab, const char *s, int len);
void abFree(struct abuf *ab);

/** output */

void editorScroll();
void editorDrawRows(struct abuf *ab);
void editorDrawStatusBar(struct abuf *ab);
void editorDrawMessageBar(struct abuf *ab);
void editorRefreshScreen();
void editorSetStatusMessage(const char *fmt, ...);

/** input */

char *editorPrompt(char *prompt, void (*callback)(char *, int));
void editorMoveCursor(int key);
void editorProcessKeypress();

/** init */

void initEditor();
int getHLDBEntries();

#endif /* KILOE_H */