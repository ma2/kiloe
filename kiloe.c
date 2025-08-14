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
#define KILO_TAB_STOP 8
#define KILO_QUIT_TIMES 3

#define CTRL_KEY(k) ((k) & 0x1f) // Ctrlキーを押したときの値を取得
#define ESC '\x1b'
#define SPC ' '

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

// ハイライト情報
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

#define HL_HIGHLIGHT_NUMBERS (1<<0) // 0ビット目が1のフラグ
#define HL_HIGHLIGHT_STRINGS (1<<1) // 1ビット目が1のフラグ

/** data */

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
  int size;     // charsのサイズ
  int rsize;    // renderのサイズ
  char *chars;  // 文字列のバッファ
  char *render; // 実際にレンダリングした結果のバッファ（TABを複数スペースに展開等）
  unsigned char *hl;  // render内の各文字のハイライト情報の配列
  int hl_open_comment;  // 前の行のコメントが閉じていない
} erow;

struct editorConfig {
  int cx, cy;     // カーソル位置
  int rx;         // renderでのカーソル位置
  int rowoff;     // 行オフセット
  int coloff;     // 列オフセット
  int screenrows; // 画面の行数
  int screencols; // 画面の列数
  int numrows;
  erow *row;      // 編集中の行バッファへのポインタ
  int dirty;      // テキストバッファが変更されているどうか
  char *filename; // 編集中ファイル名
  char statusmsg[80];
  time_t statusmsg_time;
  struct editorSyntax *syntax;
  struct termios orig_termios; // 元の端末設定
};

struct editorConfig E;

/** filetypes */

char *C_HL_extensions[] = { ".c", ".h", ".cpp", NULL };

char *C_HL_Keywords[] = {
  "switch", "if", "while", "for", "break", "continue", "return", "else",
  "struct", "union", "typedef", "static", "enum", "class", "case",
  "int|", "long|", "double|", "float|", "char|", "unsigned|", "signed|",
  "void|", NULL
};

struct editorSyntax HLDB[] = {
  {
    "c",
    C_HL_extensions,
    C_HL_Keywords,
    "//", "/*", "*/",
    HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
  },
};

// HLDB配列長
#define HLDB_ENTRIES (sizeof(HLDB) / sizeof(HLDB[0]))

/** prototypes */

void editorSetStatusMessage(const char *fmt, ...);
void editorRefreshScreen();
char *editorPrompt(char *prompt, void (*callback)(char *, int));

/** UTF-8 helpers */

// UTF-8の継続バイトかチェック
static inline int is_utf8_continuation(unsigned char c) {
    return (c & 0xC0) == 0x80;
}

// 次の文字境界へ移動
int move_to_next_char(char *str, int pos, int max) {
    if (pos >= max) return max;
    pos++;
    // 継続バイトをスキップ
    while (pos < max && is_utf8_continuation((unsigned char)str[pos])) {
        pos++;
    }
    return pos;
}

// 前の文字境界へ移動
int move_to_prev_char(char *str, int pos) {
    if (pos <= 0) return 0;
    pos--;
    // 文字の先頭を探す
    while (pos > 0 && is_utf8_continuation((unsigned char)str[pos])) {
        pos--;
    }
    return pos;
}

// UTF-8文字の表示幅を取得
int get_char_width(char *str, int pos) {
    unsigned char c = (unsigned char)str[pos];
    
    // ASCII文字は1幅
    if (c < 0x80) return 1;
    
    // 3バイトUTF-8（多くの日本語文字）は2幅
    if ((c & 0xF0) == 0xE0) return 2;
    
    // 2バイトと4バイトUTF-8は1幅（簡易実装）
    return 1;
}

/** terminal */

void die(const char *s) {
  write(STDERR_FILENO, "\x1b[2J", 4); // 画面をクリア
  write(STDERR_FILENO, "\x1b[H", 3);  // カーソルをホーム位置に移動
  perror(s);
  exit(1);
}

void disableRawMode() {
  // 元の端末設定に戻す
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1) {
    die("tcsetattr");
  }
}

void enableRawMode() {
  // 端末状態取得
  if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) {
    die("tcgetattr");
  }
  atexit(disableRawMode); // プログラム終了時に元の設定に戻す

  // 端末設定をrawモードに変更（以下のフラグをOFFにする）
  // ICRNL: 改行をキャリッジリターンに変換
  // IXON: ソフトフローコントロール（Ctrl-S/Q）を有効化
  // OPOST: 出力の後処理（改行変換など）を無効化
  // ECHO: 入力文字を表示
  // ICANON: 入力を行単位で処理
  // ISIG: シグナル(Ctrl-X/Z）を有効化
  // IEXTEN: 拡張機能（Ctrl-Vなど）を有効化
  struct termios raw = E.orig_termios;
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
  raw.c_cc[VMIN] = 0; // 最小入力文字数
  raw.c_cc[VTIME] = 1; // タイムアウト（100ms）

  // 端末状態設定
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
    die("tcsetattr");
  }
}

// キーの読み込み
int editorReadKey() {
  int nread;
  char c;
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN) {
      die("read");
    }
  }

  // 特殊キー（エスケープシーケンス）の処理
  if (c == ESC) {
    char seq[3];

    if (read(STDIN_FILENO, &seq[0], 1) != 1) return ESC;
    if (read(STDIN_FILENO, &seq[1], 1) != 1) return ESC;

    // HOMEやENDは、OSによってシーケンスが異なるので複数ある
    if (seq[0] == '[') {
      if (seq[1] >= '0' && seq[1] <= '9') {
        if (read(STDIN_FILENO, &seq[2], 1) != 1) return ESC;
        if (seq[2] == '~') {
          switch (seq[1]) {
            case '1': return HOME;
            case '3': return DELETE;
            case '4': return END;
            case '5': return PAGE_UP;
            case '6': return PAGE_DOWN;
            case '7': return HOME;
            case '8': return END;
          }
        }
      } else {
        switch (seq[1]) {
          case 'A': return ARROW_UP;
          case 'B': return ARROW_DOWN;
          case 'C': return ARROW_RIGHT;
          case 'D': return ARROW_LEFT;
          case 'H': return HOME;
          case 'F': return END;
        }
      }
    } else if (seq[0] == '0') {
      switch (seq[1]) {
        case 'H': return HOME;
        case 'F': return END;
      }
    }
    return ESC;
  } else {
    return c;
  }
}

int getCursorPosition(int *rows, int *cols) {
  char buf[32];
  unsigned int i = 0;

  // カーソル位置問い合わせ
  if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

  // 戻り値もエスケープシーケンス
  // ESC[行;列R
  while (i < sizeof(buf) - 1) {
    if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
    if (buf[i] == 'R') break;
    i++;
  }
  buf[i] = '\0';

  // エスケープシーケンスじゃなければエラー
  if (buf[0] != ESC || buf[1] != '[') return -1;
  if (sscanf(&buf[2], "%2d;%2d", rows, cols) != 2) return -1;

  return 0;
}

int getWindowSize(int *rows, int *cols) {
  struct winsize ws;
  
  // ioctlでターミナルのサイズが分からなかった場合
  // 999,999にカーソルを移動して（つまりターミナル右下の）座標を取得する
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) {
      return -1;
    }
    return getCursorPosition(rows, cols);
  } else {
    *rows = ws.ws_row;
    *cols = ws.ws_col;
    return 0; // 成功
  }
}

/** syntax highlighting */

int is_separator(int c) {
  return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}

void editorUpdateSyntax(erow *row) {
  row->hl = realloc(row->hl, row->rsize);
  // hlをHL_NORMALで埋めて初期化
  memset(row->hl, HL_NORMAL, row->rsize);

  if (E.syntax == NULL) return;

  char **keywords = E.syntax->keywords;

  char *scs = E.syntax->singleline_comment_start;
  char *mcs = E.syntax->multiline_comment_start;
  char *mce = E.syntax->multiline_comment_end;

  int scs_len = scs ? strlen(scs) : 0;
  int mcs_len = mcs ? strlen(mcs) : 0;
  int mce_len = mce ? strlen(mce) : 0;

  // ファイルの先頭はセパレータ扱いなので初期値は1(=true)
  int prev_sep  = 1;
  int in_string = 0;  // 文字列処理中
  int in_comment = (row->idx > 0 && E.row[row->idx - 1].hl_open_comment); // 複数行コメント処理中

  int i = 0;
  while (i < row->rsize) {
    char c = row->render[i];
    unsigned char prev_hl = (i > 0) ? row->hl[i - 1] : HL_NORMAL;

    // 一行コメント処理
    if (scs_len && !in_string && !in_comment) {
      // strncmp : 2つの文字列を指定文字数分比較する
      if (!strncmp(&row->render[i], scs, scs_len)) {
        memset(&row->hl[i], HL_COMMENT, row->rsize - i);
        break;
      }
    }

    // 複数行コメントハイライト処理
    if (mcs_len && mce_len && !in_string) {
      if (in_comment) {
        row->hl[i] = HL_MLCOMMENT;
        if (!strncmp(&row->render[i], mce, mce_len)) {
          memset(&row->hl[i], HL_MLCOMMENT, mce_len);
          i += mce_len;
          in_comment = 0;
          prev_sep = 1;
          continue;
        }
      } else if (!strncmp(&row->render[i], mcs, mcs_len)) {
        memset(&row->hl[i], HL_MLCOMMENT, mcs_len);
        i += mcs_len;
        in_comment = 1;
        continue;
      }
    }

    // 文字列ハイライト処理
    if (E.syntax->flags & HL_HIGHLIGHT_STRINGS) {
      if (in_string) {
        // 文字列開始している場合
        row->hl[i] = HL_STRING;
        // エスケープされている場合はスキップ
        if (c == '\\' && i + 1 < row->rsize) {
          row->hl[i + 1] = HL_STRING;
          i += 2;
          continue;
        }
        // 文字列終了
        if (c == in_string) in_string = 0;
        i++;
        prev_sep = 1;
        continue;
      } else {
        // 文字列開始していない場合
        if (c == '"' || c == '\'') {
          // 文字列開始
          in_string = c;
          row->hl[i] = HL_STRING;
          i++;
          continue;
        }
      }
    }

    // 数字ハイライト処理
    if (E.syntax->flags & HL_HIGHLIGHT_NUMBERS) {
      if ((isdigit(c) && (prev_sep || prev_hl == HL_NUMBER)) || (c == '.' && prev_hl == HL_NUMBER)) {
        // 対象文字が数字かつ直前がセパレータまたはハイライト情報がHL_NUMBERだったらHL_NUMBERにする
        // これによりセパレータから連続する数字のみハイライトされ、単語中の数字はハイライトされない
        // 対象文字がドットかつ直前ハイライト情報がHL_NUMBERだったらHL_NUMBERにする
        // これにより小数点を含んだ数字がハイライトされる
        row->hl[i] = HL_NUMBER;
        i++;
        prev_sep = 0;
        continue;
      }
    }

    // キーワードのハイライト処理
    if (prev_sep) {
      // 直前にセパレータが必要
      int j;
      for (j = 0; keywords[j]; j++) {
        // キーワードを順に取得（キーワード2ならkw2が）
        int klen = strlen(keywords[j]);
        int kw2 = keywords[j][klen - 1] == '|';
        if (kw2) klen--;

        // キーワードと行を比較
        // strncmp : 2つの文字列を指定文字数分比較する
        if (!strncmp(&row->render[i], keywords[j], klen) && is_separator(row->render[i + klen])) {
          memset(&row->hl[i], kw2 ? HL_KEYWORD2 : HL_KEYWORD1, klen);
          i += klen;
          break;
        }
      }
      if (keywords[j] != NULL) {
        prev_sep = 0;
        continue;
      }
    }

    prev_sep = is_separator(c);
    i++;
  }

  int changed = (row->hl_open_comment != in_comment);
  row->hl_open_comment = in_comment;
  if (changed && row->idx + 1 < E.numrows) {
    editorUpdateSyntax(&E.row[row->idx + 1]);
  }
}

// シンタックスハイライトの色
int editorSyntaxToColor(int hl) {
  switch (hl) {
    case HL_COMMENT:
    case HL_MLCOMMENT: return 36;   // シアン
    case HL_KEYWORD1: return 33;  // 黄色
    case HL_KEYWORD2: return 32;  // 緑
    case HL_STRING: return 35;    // マゼンタ
    case HL_NUMBER: return 31;    // 赤
    case HL_MATCH: return 34;     // 青
    default: return 37;
  }
}

// 適切なファイルハイライト設定を選択する
void editorSelectSyntaxHighlight() {
  E.syntax = NULL;
  if (E.filename == NULL) return;

  char *ext = strrchr(E.filename, '.');

  for (unsigned int j = 0; j < HLDB_ENTRIES; j++) {
    struct editorSyntax *s = &HLDB[j];
    unsigned int i = 0;
    while (s->filematch[i]) {
      int is_ext = (s->filematch[i][0] == '.');
      if ((is_ext && ext && !strcmp(ext, s->filematch[i])) || (!is_ext && strstr(E.filename, s->filematch[i]))) {
        E.syntax = s;

        int filerow;
        for (filerow = 0; filerow < E.numrows; filerow++) {
          editorUpdateSyntax(&E.row[filerow]);
        }

        return;
      }
      i++;
    }
  }
}

/** row operations  */

// chars内カーソル位置をrender内カーソル位置に変換
int editorRowCxToRx(erow *row, int cx) {
  int rx = 0;
  int j = 0;
  
  while (j < cx && j < row->size) {
    if (row->chars[j] == '\t') {
      rx += (KILO_TAB_STOP - 1) - (rx % KILO_TAB_STOP);
      rx++;
      j++;
    } else {
      // UTF-8文字の表示幅を考慮
      int char_width = get_char_width(row->chars, j);
      rx += char_width;
      // 次の文字の開始位置へ移動
      j = move_to_next_char(row->chars, j, row->size);
    }
  }
  return rx;
}

// render内カーソル位置をchars内カーソル位置に変換
int editorRowRxToCx(erow *row, int rx) {
  int cur_rx = 0;
  int cx = 0;
  
  while (cx < row->size) {
    if (row->chars[cx] == '\t') {
      cur_rx += (KILO_TAB_STOP - 1) - (cur_rx % KILO_TAB_STOP);
      cur_rx++;
      if (cur_rx > rx) return cx;
      cx++;
    } else {
      // UTF-8文字の表示幅を考慮
      int char_width = get_char_width(row->chars, cx);
      if (cur_rx + char_width > rx) return cx;
      cur_rx += char_width;
      // 次の文字の開始位置へ移動
      cx = move_to_next_char(row->chars, cx, row->size);
    }
  }
  return cx;
}

void editorUpdateRow(erow *row) {
  int tabs = 0;
  int j;
  for (j = 0; j < row->size; j++) {
    if (row->chars[j] == '\t') tabs++;
  }

  free(row->render);
  row->render = malloc(row->size + tabs * (KILO_TAB_STOP - 1) + 1);

  int idx = 0;
  for (j = 0; j < row->size; j++) {
    // TABなら空白8つに置き換え
    if (row->chars[j] == '\t') {
      row->render[idx++] = SPC;
      while (idx % KILO_TAB_STOP != 0) row->render[idx++] = SPC;
    } else {
      row->render[idx++] = row->chars[j];
    }
  }
  row->render[idx] = '\0';
  row->rsize = idx;

  editorUpdateSyntax(row);
}

// 行を追加
void editorInsertRow(int at, char *s, size_t len) {
  if (at < 0 || at > E.numrows) return;

  E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
  memmove(&E.row[at + 1], &E.row[at], sizeof(erow) * (E.numrows - at));
  for (int j = at + 1; j<= E.numrows; j++) E.row[j].idx++;  // 行のインデックスを更新

  E.row[at].idx = at;

  E.row[at].size = len;
  E.row[at].chars = malloc(len + 1);
  memcpy(E.row[at].chars, s, len);
  E.row[at].chars[len] = '\0';
  
  E.row[at].rsize = 0;
  E.row[at].render = NULL;
  E.row[at].hl = NULL;
  E.row[at].hl_open_comment = 0;
  editorUpdateRow(&E.row[at]);
  
  E.numrows++;
  E.dirty++;
}

// 行のメモリを解放する
void editorFreeRow(erow *row) {
  free(row->render);
  free(row->chars);
  free(row->hl);
}

// 行を削除する
void editorDelRow(int at) {
  if (at < 0 || at >= E.numrows) return;
  editorFreeRow(&E.row[at]);
  memmove(&E.row[at], &E.row[at + 1], sizeof(erow) * (E.numrows - at - 1));
  E.numrows--;
  E.dirty++;
}

// 行に文字をインサート
void editorRowInsertChar(erow *row, int at, int c) {
  if (at < 0 || at > row->size) at = row->size;
  row->chars = realloc(row->chars, row->size + 2);
  // memmoveはsrcとdstが重なっても安全
  memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
  row->size++;
  row->chars[at] = c;
  editorUpdateRow(row);
  E.dirty++;
}

// 行末尾に文字列をアペンド
void editorRowAppendString(erow *row, char *s, size_t len) {
  row->chars = realloc(row->chars, row->size + len + 1);
  memcpy(&row->chars[row->size], s, len);
  row->size += len;
  row->chars[row->size] = '\0';
  editorUpdateRow(row);
  E.dirty++;
}

// 行から指定直前の文字を削除する
void editorRowDelChar(erow *row, int at) {
  if (at < 0 || at >= row->size) return;
  // at番目のアドレスに、at+1番目のアドレスの内容をコピーする
  memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
  row->size--;
  editorUpdateRow(row);
  E.dirty++;
}

/** editor operations */

// カーソル位置に文字をインサート
void editorInsertChar(int c) {
  // 最終行なら新規行を追加する
  if (E.cy == E.numrows) {
    editorInsertRow(E.numrows, "", 0);
  }
  editorRowInsertChar(&E.row[E.cy], E.cx, c);
  E.cx++;
}

// カーソルの位置で改行
void editorInsertNewLine() {
  if (E.cx == 0) {
    editorInsertRow(E.cy, "", 0);
  } else {
    erow *row = &E.row[E.cy];
    editorInsertRow(E.cy + 1, &row->chars[E.cx], row->size - E.cx);
    row = &E.row[E.cy];
    row->size = E.cx;
    row->chars[row->size] = '\0';
    editorUpdateRow(row);
  }
  E.cy++;
  E.cx = 0;
}

// カーソル直前の文字を削除
void editorDelChar() {
  if (E.cy == E.numrows) return;
  if (E.cx == 0 && E.cy == 0) return;

  erow *row = &E.row[E.cy];
  if (E.cx > 0) {
    editorRowDelChar(row, E.cx - 1);
    E.cx--;
  } else {
    // 行頭でのBS→現在行を全行の後ろにアペンドする
    E.cx = E.row[E.cy - 1].size;
    editorRowAppendString(&E.row[E.cy - 1], row->chars, row->size);
    editorDelRow(E.cy);
    E.cy--;
  }
}

/** file io */

// 編集テキストを1行の文字列に変換する
char *editorRowsToString(int *buflen) {
  int totlen = 0;
  int j;
  for (j = 0; j < E.numrows; j++) {
    totlen += E.row[j].size + 1;
  }
  *buflen = totlen;

  char *buf = malloc(totlen);
  char *p = buf;
  for (j=0; j < E.numrows; j++) {
    memcpy(p, E.row[j].chars, E.row[j].size);
    p += E.row[j].size;
    *p = '\n';
    p++;
  }

  return buf;
}

// ファイルを読み込んで編集テキストにセットする
void editorOpen(char *filename) {
  free(E.filename);
  E.filename = strdup(filename);

  editorSelectSyntaxHighlight();

  FILE *fp = fopen(filename, "r");
  if (!fp) die ("fopen");

  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;
  // 改行文字までを読み込んで文字列として返す
  // &lineがNULLで&linecapが0のとき、読み込んだ文字列に応じて自動的にmallocする
  // line: 読み込んだ行のバッファ
  // linecap: バッファのバイト数
  while ((linelen = getline(&line, &linecap, fp)) != -1) {
    // 行末の改行文字を除いた文字数をカウント
    while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) {
      linelen--;
    }
    editorInsertRow(E.numrows, line, linelen);
  }
  // 自動確保されたlineはfreeする必要がある
  free(line);
  fclose(fp);
  E.dirty = 0;
}

void editorSave() {
  if (E.filename == NULL) {
    E.filename = editorPrompt("Save as: %s (ESC to cancel)", NULL);
    if (E.filename == NULL) {
      editorSetStatusMessage("Save aborted");
      return;
    }

    editorSelectSyntaxHighlight();
  }

  int len;
  char *buf = editorRowsToString(&len);
  int fd = open(E.filename, O_RDWR | O_CREAT, 0644);

  if (fd != -1) {
    if (ftruncate(fd, len) != -1) {
      if (write(fd, buf, len) != -1) {
        close(fd);
        free(buf);
        E.dirty = 0;
        editorSetStatusMessage("%d bytes written to disk", len);
        return;
      }
    }
    close(fd);
  }

  free(buf);
  editorSetStatusMessage("Can't save! I/O error: %s", strerror(errno));
}

/** find */

void editorFindCallback(char *query, int key) {
  static int last_match = -1; // 最後にマッチした行のインデックス
  static int direction = 1;   // 検索方向（1: 順方向、-1: 逆方向）

  static int saved_hl_line;
  static char *saved_hl = NULL;

  if (saved_hl) {
    memcpy(E.row[saved_hl_line].hl, saved_hl, E.row[saved_hl_line].rsize);
    free(saved_hl);
    saved_hl = NULL;
  }

  if (key == '\r' || key == ESC) {
    last_match = -1;
    direction = 1;
    return;
  } else if (key == ARROW_RIGHT || key == ARROW_DOWN) {
    direction = 1;
  } else if (key == ARROW_LEFT || key == ARROW_UP) {
    direction = -1;
  } else {
    last_match = -1;
    direction = 1;
  }

  if (last_match == -1) direction = 1;
  int current = last_match;

  int i;
  for (i = 0; i < E.numrows; i++) {
    current += direction;
    if (current == -1) current = E.numrows - 1;
    else if (current == E.numrows) current = 0;

    erow *row = &E.row[current];
    char *match = strstr(row->render, query);
    if (match) {
      last_match = current;
      E.cy = current;
      // match - row->renderはrender内でのmatchまでのバイト数=rxになる
      E.cx = editorRowRxToCx(row, match - row->render);
      // マッチした行を先頭にする
      E.rowoff = E.numrows;

      // ハイライト前の状態を保存
      saved_hl_line = current;
      saved_hl = malloc(row->rsize);
      memcpy(saved_hl, row->hl, row->rsize);

      // findした文字列をハイライト
      memset(&row->hl[match - row->render], HL_MATCH, strlen(query));
      break;
    }
  }
}

void editorFind() {
  int saved_cx = E.cx;
  int saved_cy = E.cy;
  int save_coloff = E.coloff;
  int saved_rowoff = E.rowoff;

  char *query = editorPrompt("Search: %s (ESC/Arrows/Enter)", editorFindCallback);
  if (query == NULL) return;

  if (query) {
    free(query);
  } else {
    // カーソル位置を復元
    E.cx = saved_cx;
    E.cy = saved_cy;
    E.coloff = save_coloff;
    E.rowoff = saved_rowoff;
  }
}

/** append buffer */

struct abuf {
  char *b;
  int len;
};

#define ABUF_INIT {NULL, 0}

// アペンドバッファに文字列sを追加
// abuf: アペンドバッファ情報
// s: 文字列
// len: 文字列長
void abAppend(struct abuf *ab, const char *s, int len) {
  // 元のバッファを拡張
  char *new = realloc(ab->b, ab->len + len);
  if (new == NULL) return;
  // 拡張したところにsをコピー
  memcpy(&new[ab->len], s, len);
  ab->b = new;
  ab->len += len;
}

void abFree(struct abuf *ab) {
  free(ab->b);
}

/** output */

void editorScroll() {
  E.rx = E.cx;

  if (E.cy < E.numrows) {
    E.rx = editorRowCxToRx(&E.row[E.cy], E.cx);
  }
  // カーソルが画面上にあれば、行オフセット=カーソル位置
  if (E.cy < E.rowoff) {
    E.rowoff = E.cy;
  }
  // カーソルが最終行を越えていれば、行オフセット=カーソル位置-画面の行数
  if (E.cy >= E.rowoff + E.screenrows) {
    E.rowoff = E.cy - E.screenrows + 1;
  }
  // カーソルが画面上にあれば、列オフセット=カーソル位置
  if (E.rx < E.coloff) {
    E.coloff = E.rx;
  }
  // カーソルが最終列を越えていれば、列オフセット=カーソル位置-画面の列数
  if (E.rx >= E.coloff + E.screencols) {
    E.coloff = E.rx - E.screencols + 1;
  }
}

void editorDrawRows(struct abuf *ab) {
  int y;
  for (y = 0; y < E.screenrows; y++) {
    int filerow = y + E.rowoff;
    if (filerow >= E.numrows) {
      if (E.numrows == 0 && y == E.screenrows / 3) {
        char welcome[80];
        int welcomelen = snprintf(welcome, sizeof(welcome), "Kilo editor -- version %s", KILO_VERSION);
        if (welcomelen > E.screencols) welcomelen = E.screencols;
        // 画面中央にwelcome表示
        int padding = (E.screencols - welcomelen) / 2;
        if (padding) {
          abAppend(ab, "~", 1);
          padding--;
        }
        while (padding--) abAppend(ab, " ", 1);
        abAppend(ab, welcome, welcomelen);
      } else {
        abAppend(ab, "~", 1);   // 各行にチルダを表示
      }
    } else {
      int len = E.row[filerow].rsize - E.coloff;
      if (len < 0) len = 0;
      if (len > E.screencols) len = E.screencols;
      char *c = &E.row[filerow].render[E.coloff];
      unsigned char *hl = &E.row[filerow].hl[E.coloff];
      int current_color = -1;
      int j;
      for (j = 0; j < len; j++) {
        // UTF-8の場合、0x80以上のバイトは制御文字ではない
        if ((unsigned char)c[j] < 0x80 && iscntrl(c[j])) {
          // 制御コードの場合
          char sym = (c[j] <= 26) ? '@' + c[j] : '?';
          abAppend(ab, "\x1b[7m", 4);
          abAppend(ab, &sym, 1);
          abAppend(ab, "\x1b[m", 3);
          if (current_color != -1) {
            char buf[16];
            int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", current_color);
            abAppend(ab, buf, clen);
          }
        } else if (hl[j] == HL_NORMAL) {
          if (current_color != -1) {
            // 通常色になっていなければリセットする
            abAppend(ab, "\x1b[39m", 5);
            current_color = -1;
          }
          abAppend(ab, &c[j], 1);
        } else {
          int color = editorSyntaxToColor(hl[j]);
          if (color != current_color) {
            // 前の文字とカラーが変化したときのみエスケープシーケンスを挿入する
            current_color = color;
            char buf[16];
            int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", color);
            abAppend(ab, buf, clen);
          }
          abAppend(ab, &c[j], 1);
        }
      }
      abAppend(ab, "\x1b[39m", 5);
    }
    
    abAppend(ab, "\x1b[K", 3); // 行末まで削除
    abAppend(ab, "\r\n", 2);   // 改行
  }
}

void editorDrawStatusBar(struct abuf *ab) {
  // 表示を反転
  abAppend(ab, "\x1b[7m", 4);
  char status[80];  // ステータスバー
  char rstatus[80]; // ステータスバー右端
  // ファイル名とファイルの行数
  int len = snprintf(status, sizeof(status), "%.20s - %d lines %s", E.filename ? E.filename : "[No Name]", E.numrows, E.dirty ? "(modified)" : "");
  // 現在の行数、ファイルタイプ
  int rlen = snprintf(rstatus, sizeof(rstatus), "%s | %d/%d", E.syntax ? E.syntax->filetype : "no ft", E.cy + 1, E.numrows);
  if (len > E.screencols) len = E.screencols;
  abAppend(ab, status, len);
  while (len < E.screencols) {
    // 1行ぴったりになるようにstatusとrstatusをくっつける
    if (E.screencols - len == rlen) {
      abAppend(ab, rstatus, rlen);
      break;
    } else {
      abAppend(ab, " ", 1);
      len++;
    }
  }
  // 反転表示を元に戻す
  abAppend(ab, "\x1b[m", 3);
  abAppend(ab, "\r\n", 2);
}

void editorDrawMessageBar(struct abuf *ab) {
  abAppend(ab, "\x1b[K", 3);
  int msglen = strlen(E.statusmsg);
  if (msglen > E.screencols) msglen = E.screencols;
  if (msglen && time(NULL) - E.statusmsg_time < 5) {
    abAppend(ab, E.statusmsg, msglen);
  }
}

void editorRefreshScreen() {
  editorScroll();

  struct abuf ab = ABUF_INIT;

  abAppend(&ab, "\x1b[?25l", 6); // カーソルOFF
  abAppend(&ab, "\x1b[H", 3);  // カーソルをホーム位置に移動

  editorDrawRows(&ab);
  editorDrawStatusBar(&ab);
  editorDrawMessageBar(&ab);

  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy - E.rowoff + 1, E.rx - E.coloff  + 1);
  abAppend(&ab, buf, strlen(buf));

  abAppend(&ab, "\x1b[?25h", 6); // カーソルON

  // アペンドバッファの内容を描画
  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);
}

void editorSetStatusMessage(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
  va_end(ap);
  E.statusmsg_time = time(NULL);
}

/** input */

char *editorPrompt(char *prompt, void (*callback)(char *, int)) {
  size_t bufsize = 128;
  char *buf = malloc(bufsize);  // ユーザ入力を保持するバッファ

  size_t buflen = 0;
  buf[0] = '\0';

  while(1) {
    editorSetStatusMessage(prompt, buf);
    editorRefreshScreen();

    int c = editorReadKey();
    if (c == DELETE || c == CTRL_KEY('h') || c == BACKSPACE) {
      // バッファ内でBSを有効にする
      if (buflen != 0) buf[--buflen] = '\0';
    } else if (c == ESC) {
      // ESCを入力したらバッファをクリア
      editorSetStatusMessage("");
      if (callback) callback(buf, c);
      free(buf);
      return NULL;
    } else if (c == '\r') {
      // ENTERを入力したらバッファを返す
      if (buflen != 0) {
        editorSetStatusMessage("");
        if (callback) callback(buf, c);
        return buf;
      }
    } else if (!iscntrl(c) && c < 128) {
      if (buflen == bufsize - 1) {
        // バッファがいっぱいなったら倍に拡張
        bufsize *= 2;
        buf = realloc(buf, bufsize);
      }
      // 制御コード以外ならバッファに追加
      buf[buflen++] = c;
      buf[buflen] = '\0';
    }

    if (callback) callback(buf, c);
  }
}

void editorMoveCursor(int key) {
  erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];

  switch(key) {
    case ARROW_LEFT:
      if (E.cx != 0) {
        // UTF-8文字境界を考慮して前の文字へ移動
        E.cx = move_to_prev_char(row->chars, E.cx);
      } else if (E.cy > 0) {
        // 行頭で←を押したら、前行の行末に移動
        E.cy--;
        E.cx = E.row[E.cy].size;
      }
      break;
    case ARROW_RIGHT:
    // カーソルは行端より右には移動しない
      if (row && E.cx < row->size) {
        // UTF-8文字境界を考慮して次の文字へ移動
        E.cx = move_to_next_char(row->chars, E.cx, row->size);
      } else if (row && E.cx == row->size) {
        // 行末で→を押したら、次行の先頭に移動
        E.cy++;
        E.cx = 0;
      }
      break;
    case ARROW_UP:
    case ARROW_DOWN: {
      // 上下移動時は現在の表示位置(rx)を記憶
      int target_rx = E.rx;
      if (E.cy < E.numrows) {
        target_rx = editorRowCxToRx(&E.row[E.cy], E.cx);
      }
      
      if (key == ARROW_UP && E.cy != 0) {
        E.cy--;
      } else if (key == ARROW_DOWN && E.cy < E.numrows) {
        E.cy++;
      }
      
      // 新しい行で同じ表示位置に最も近いバイト位置を計算
      row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
      if (row) {
        E.cx = editorRowRxToCx(row, target_rx);
      } else {
        E.cx = 0;
      }
      break;
    }
  }
  row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
  int rowlen = row ? row->size : 0;
  if (E.cx > rowlen) {
    E.cx = rowlen;
  }
}

void editorProcessKeypress() {
  static int quit_times = KILO_QUIT_TIMES;
  int c = editorReadKey();
  switch (c) {
    case '\r':
      // Enterキー
      editorInsertNewLine();
      break;

    case CTRL_KEY('q'):
      if (E.dirty && quit_times > 0) {
        editorSetStatusMessage("WARNING!!! FIle has unsaved changes. "
        "Press Ctrl-q %d times to quit.", quit_times);
        quit_times--;
        return;
      }
      write(STDOUT_FILENO, "\x1b[2J", 4);
      write(STDOUT_FILENO, "\x1b[H", 3);
      exit(0);
      break;

    case CTRL_KEY('s'):
      editorSave();
      break;

    case HOME:
      E.cx = 0;
      break;

    case END:
      if (E.cy < E.numrows) {
        E.cx = E.row[E.cy].size;
      }
      break;
    
    case CTRL_KEY('f'):
      editorFind();
      break;

    case BACKSPACE:
    case CTRL_KEY('h'):
    case DELETE:
      // DELETEはカーソルを右に動かしてからBSする
      if (c == DELETE) editorMoveCursor(ARROW_RIGHT);
      editorDelChar();
      break;

    case PAGE_UP:
    case PAGE_DOWN:
      {
        if (c == PAGE_UP) {
          E.cy = E.rowoff;
        } else if (c == PAGE_DOWN) {
          E.cy = E.rowoff + E.screenrows - 1;
          if (E.cy > E.numrows) E.cy = E.numrows;
        }

        int times = E.screenrows;
        while (times--) {
          editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
        }
      }
      break;

    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
      editorMoveCursor(c);
      break;

    case CTRL_KEY('l'):
    case ESC:
      break;

    default:
      editorInsertChar(c);
      break;
  }

  quit_times = KILO_QUIT_TIMES;
}

/** init */

void initEditor() {
  E.cx = 0;
  E.cy = 0;
  E.rx = 0;
  E.rowoff = 0;
  E.coloff = 0;
  E.numrows = 0;
  E.row = NULL;
  E.dirty = 0;
  E.filename = NULL;
  E.statusmsg[0] = '\0';
  E.statusmsg_time = 0;
  E.syntax = NULL;

  if (getWindowSize(&E.screenrows, &E.screencols) == -1) {
    die("getWindowSize");
  }
  E.screenrows -= 2;  // ステータス行、メッセージ行を確保
}

int main(int argc, char *argv[]) {
  enableRawMode();
  initEditor();
  if (argc >= 2) {
    editorOpen(argv[1]);
  }

  editorSetStatusMessage("HELP: Ctrl-s = save | Ctrl-q = quit | Ctrl-f = find");

  while (1) {
    editorRefreshScreen();
    editorProcessKeypress();
  }
  return 0;
}