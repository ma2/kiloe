/**
 * kiloe.h - Kiloeテキストエディタ用ヘッダファイル
 * 
 * UTF-8サポートとシンタックスハイライト機能を持つ
 * ターミナルベースのテキストエディタの全データ構造、定数、関数プロトタイプを含む
 */

#ifndef KILOE_H
#define KILOE_H

/** インクルード */

/* GNU/Linux拡張機能のための機能テストマクロ */
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

/* 標準Cライブラリヘッダ */
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

/** 定義 */

/* アプリケーションバージョンと設定マクロ */
#define KILO_VERSION "0.0.1"
#define KILO_TAB_STOP (Config.tab_stop)        /* 設定可能なタブ幅 */
#define KILO_QUIT_TIMES (Config.quit_times)    /* 終了確認回数 */

/* キー操作マクロ */
#define CTRL_KEY(k) ((k) & 0x1f)   /* キーをCtrl+keyに変換 */
#define ESC '\x1b'                  /* エスケープ文字 */
#define SPC ' '                     /* スペース文字 */

/* バッファ初期化と配列サイズマクロ */
#define ABUF_INIT {NULL, 0}         /* 追加バッファの初期化 */
#define HLDB_ENTRIES (getHLDBEntries()) /* 動的シンタックスハイライトデータベースサイズ */

/* エディタキー定義 - 特殊キーを識別するための定数 */
enum editorKey {
  BACKSPACE = 127,    /* バックスペースキー */
  ARROW_LEFT = 1000,  /* 左矢印キー */
  ARROW_RIGHT,        /* 右矢印キー */
  ARROW_UP,           /* 上矢印キー */
  ARROW_DOWN,         /* 下矢印キー */
  DELETE,             /* Deleteキー */
  HOME,               /* Homeキー */
  END,                /* Endキー */
  PAGE_UP,            /* PageUpキー */
  PAGE_DOWN           /* PageDownキー */
};

/* シンタックスハイライト種別 - テキストの色分け表示用 */
enum editorHighLight {
  HL_NORMAL = 0,      /* 通常テキスト */
  HL_COMMENT,         /* コメント */
  HL_MLCOMMENT,       /* 複数行コメント */
  HL_KEYWORD1,        /* キーワード1（予約語） */
  HL_KEYWORD2,        /* キーワード2（型名） */
  HL_STRING,          /* 文字列リテラル */
  HL_NUMBER,          /* 数値リテラル */
  HL_MATCH            /* 検索マッチ */
};

/* ハイライト機能フラグ */
#define HL_HIGHLIGHT_NUMBERS (1<<0)  /* 数値のハイライト有効 */
#define HL_HIGHLIGHT_STRINGS (1<<1)  /* 文字列のハイライト有効 */

/** データ構造 */

/* シンタックスハイライト定義構造体 */
struct editorSyntax {
  char *filetype;                   /* ファイル種別名 */
  char **filematch;                 /* ファイル拡張子パターン */
  char **keywords;                  /* キーワードリスト */
  char *singleline_comment_start;   /* 単行コメント開始文字列 */
  char *multiline_comment_start;    /* 複数行コメント開始文字列 */
  char *multiline_comment_end;      /* 複数行コメント終了文字列 */
  int flags;                        /* ハイライト機能フラグ */
};

/* エディタ行構造体 - テキストの各行を表現 */
typedef struct erow {
  int idx;                  /* 行インデックス */
  int size;                 /* 文字数（バイト数） */
  int rsize;                /* 表示文字数 */
  char *chars;              /* 実際の文字データ */
  char *render;             /* 表示用文字データ（タブ展開済み） */
  unsigned char *hl;        /* ハイライト情報配列 */
  int hl_open_comment;      /* 複数行コメント開始フラグ */
} erow;

/* エディタメイン設定構造体 - エディタの状態を管理 */
struct editorConfig {
  int cx, cy;                       /* カーソル位置（文字単位） */
  int rx;                           /* 表示カーソル位置（表示単位） */
  int rowoff;                       /* 垂直スクロールオフセット */
  int coloff;                       /* 水平スクロールオフセット */
  int screenrows;                   /* 画面行数 */
  int screencols;                   /* 画面列数 */
  int numrows;                      /* ファイル総行数 */
  erow *row;                        /* 行データ配列 */
  int dirty;                        /* 変更フラグ */
  char *filename;                   /* ファイル名 */
  char statusmsg[80];               /* ステータスメッセージ */
  time_t statusmsg_time;            /* ステータスメッセージ表示時刻 */
  struct editorSyntax *syntax;      /* 使用中のシンタックスハイライト */
  struct termios orig_termios;      /* 元のターミナル設定 */
};

/* エディタ設定構造体 - 設定ファイルから読み込み可能な設定項目 */
struct editorSettings {
  int tab_stop;                     /* タブ幅 */
  int quit_times;                   /* 終了確認回数 */
  int show_line_numbers;            /* 行番号表示フラグ */
  char welcome_message[256];        /* ウェルカムメッセージ */
  int status_timeout;               /* ステータス表示タイムアウト（秒） */
  int color_comment;                /* コメントの色 */
  int color_keyword1;               /* キーワード1の色 */
  int color_keyword2;               /* キーワード2の色 */
  int color_string;                 /* 文字列の色 */
  int color_number;                 /* 数値の色 */
  int color_match;                  /* 検索マッチの色 */
};

/* 追加バッファ構造体 - 効率的な文字列構築用 */
struct abuf {
  char *b;                          /* バッファデータ */
  int len;                          /* データ長 */
};

/** グローバル変数 */

extern struct editorConfig E;        /* メインエディタ設定 */
extern struct editorSettings Config; /* ユーザー設定 */
extern struct editorSyntax HLDB[];   /* シンタックスハイライトデータベース */

/** ターミナル関数 */

void die(const char *s);
void disableRawMode();
void enableRawMode();
int editorReadKey();
int getCursorPosition(int *rows, int *cols);
int getWindowSize(int *rows, int *cols);

/** UTF-8関数 */

int is_utf8_continuation(unsigned char c);
int move_to_next_char(char *str, int pos, int max);
int move_to_prev_char(char *str, int pos);
int utf8_char_len(unsigned char c);
int get_char_width(char *str, int pos);

/** 設定関数 */

void initDefaultConfig();
int loadConfig(const char *filename);

/** シンタックスハイライト関数 */

int is_separator(int c);
void editorUpdateSyntax(erow *row);
int editorSyntaxToColor(int hl);
void editorSelectSyntaxHighlight();

/** 行操作関数 */

int editorRowCxToRx(erow *row, int cx);
int editorRowRxToCx(erow *row, int rx);
void editorUpdateRow(erow *row);
void editorInsertRow(int at, char *s, size_t len);
void editorFreeRow(erow *row);
void editorDelRow(int at);
void editorRowInsertChar(erow *row, int at, int c);
void editorRowAppendString(erow *row, char *s, size_t len);
void editorRowDelChar(erow *row, int at);

/** エディタ操作関数 */

void editorInsertChar(int c);
void editorInsertNewLine();
void editorDelChar();

/** ファイル入出力関数 */

char *editorRowsToString(int *buflen);
void editorOpen(char *filename);
void editorSave();

/** 検索関数 */

void editorFindCallback(char *query, int key);
void editorFind();

/** 追加バッファ関数 */

void abAppend(struct abuf *ab, const char *s, int len);
void abFree(struct abuf *ab);

/** 出力関数 */

void editorScroll();
void editorDrawRows(struct abuf *ab);
void editorDrawStatusBar(struct abuf *ab);
void editorDrawMessageBar(struct abuf *ab);
void editorRefreshScreen();
void editorSetStatusMessage(const char *fmt, ...);

/** 入力関数 */

char *editorPrompt(char *prompt, void (*callback)(char *, int));
void editorMoveCursor(int key);
void editorProcessKeypress();

/** 初期化関数 */

void initEditor();
int getHLDBEntries();

#endif /* KILOE_H */