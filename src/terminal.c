/**
 * terminal.c - ターミナル制御関連の機能
 * 
 * Raw modeの管理、キー入力処理、画面サイズ取得などの
 * 低レベルターミナル操作を担当
 */

#include "kiloe.h"

/* グローバル変数はmain.cで定義 */

/**
 * die - エラー時の緊急終了処理
 * @s: エラーメッセージ
 * 
 * 画面をクリアしてエラーメッセージを表示し、プログラムを終了する
 */
void die(const char *s) {
  write(STDERR_FILENO, "\x1b[2J", 4);  // 画面全体をクリア
  write(STDERR_FILENO, "\x1b[H", 3);   // カーソルをホーム位置に移動
  perror(s);
  exit(1);
}

/**
 * disableRawMode - Raw modeを無効化し、元の端末設定に戻す
 * 
 * プログラム終了時に自動的に呼ばれ、端末を通常モードに復帰させる
 */
void disableRawMode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1) {
    die("tcsetattr");
  }
}

/**
 * enableRawMode - 端末をRaw modeに設定
 * 
 * Raw modeでは入力を1文字ずつ処理し、特殊キーの処理を自前で行う
 * 以下のフラグを無効化:
 * - ICRNL: 改行をキャリッジリターンに変換
 * - IXON: ソフトフローコントロール（Ctrl-S/Q）
 * - OPOST: 出力の後処理（改行変換など）
 * - ECHO: 入力文字のエコーバック
 * - ICANON: 正規モード（行単位での入力処理）
 * - ISIG: シグナル（Ctrl-C/Z）
 * - IEXTEN: 拡張入力処理（Ctrl-Vなど）
 */
void enableRawMode() {
  // 現在の端末設定を保存
  if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) {
    die("tcgetattr");
  }
  // プログラム終了時に元の設定に戻すよう登録
  atexit(disableRawMode);

  // Raw mode用の設定を作成
  struct termios raw = E.orig_termios;
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
  raw.c_cc[VMIN] = 0;   // 最小入力文字数（0=非ブロッキング）
  raw.c_cc[VTIME] = 1;  // タイムアウト（100ms単位）

  // 新しい設定を適用
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
    die("tcsetattr");
  }
}

/**
 * editorReadKey - キーボードから1文字（または特殊キー）を読み取る
 * 
 * 通常の文字はそのまま返し、矢印キーやHome/Endなどの特殊キーは
 * エスケープシーケンスを解析して専用の定数を返す
 * 
 * @return: 入力されたキーコード（特殊キーの場合は定数値）
 */
int editorReadKey() {
  int nread;
  char c;
  
  // 1文字読み取るまでループ
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN) {
      die("read");
    }
  }

  // エスケープシーケンスの処理
  if (c == ESC) {
    char seq[3];

    // エスケープシーケンスの残りを読み取る
    if (read(STDIN_FILENO, &seq[0], 1) != 1) return ESC;
    if (read(STDIN_FILENO, &seq[1], 1) != 1) return ESC;

    // ESC [ で始まるシーケンス
    if (seq[0] == '[') {
      // ESC [ 数字 ~ 形式（Page Up/Down, Delete等）
      if (seq[1] >= '0' && seq[1] <= '9') {
        if (read(STDIN_FILENO, &seq[2], 1) != 1) return ESC;
        if (seq[2] == '~') {
          switch (seq[1]) {
            case '1': return HOME;      // ESC[1~
            case '3': return DELETE;    // ESC[3~
            case '4': return END;       // ESC[4~
            case '5': return PAGE_UP;   // ESC[5~
            case '6': return PAGE_DOWN; // ESC[6~
            case '7': return HOME;      // ESC[7~ (代替)
            case '8': return END;       // ESC[8~ (代替)
          }
        }
      } else {
        // ESC [ 文字 形式（矢印キー等）
        switch (seq[1]) {
          case 'A': return ARROW_UP;    // ESC[A
          case 'B': return ARROW_DOWN;  // ESC[B
          case 'C': return ARROW_RIGHT; // ESC[C
          case 'D': return ARROW_LEFT;  // ESC[D
          case 'H': return HOME;        // ESC[H
          case 'F': return END;         // ESC[F
        }
      }
    } else if (seq[0] == '0') {
      // ESC 0 形式（一部の端末）
      switch (seq[1]) {
        case 'H': return HOME;  // ESC0H
        case 'F': return END;   // ESC0F
      }
    }
    return ESC;
  } else {
    return c;
  }
}

/**
 * getCursorPosition - 現在のカーソル位置を取得
 * @rows: 行位置を格納するポインタ
 * @cols: 列位置を格納するポインタ
 * 
 * ANSI エスケープシーケンス ESC[6n を使用してカーソル位置を問い合わせ、
 * 端末からの応答 ESC[行;列R を解析する
 * 
 * @return: 成功時0、失敗時-1
 */
int getCursorPosition(int *rows, int *cols) {
  char buf[32];
  unsigned int i = 0;

  // カーソル位置問い合わせ（Device Status Report）
  if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

  // 応答を読み取る（ESC[行;列R 形式）
  while (i < sizeof(buf) - 1) {
    if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
    if (buf[i] == 'R') break;  // 終端文字
    i++;
  }
  buf[i] = '\0';

  // 応答の解析
  if (buf[0] != ESC || buf[1] != '[') return -1;
  if (sscanf(&buf[2], "%2d;%2d", rows, cols) != 2) return -1;

  return 0;
}

/**
 * getWindowSize - ターミナルウィンドウのサイズを取得
 * @rows: 行数を格納するポインタ
 * @cols: 列数を格納するポインタ
 * 
 * まずioctlシステムコールでサイズ取得を試み、
 * 失敗した場合はカーソルを右下端に移動してその位置を取得する
 * 
 * @return: 成功時0、失敗時-1
 */
int getWindowSize(int *rows, int *cols) {
  struct winsize ws;
  
  // ioctlでウィンドウサイズ取得を試みる
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    // 失敗時の代替手段：カーソルを右下端（999,999）に移動
    if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) {
      return -1;
    }
    // 移動後の位置を取得（実際のウィンドウサイズ）
    return getCursorPosition(rows, cols);
  } else {
    // ioctlが成功した場合
    *rows = ws.ws_row;
    *cols = ws.ws_col;
    return 0;
  }
}