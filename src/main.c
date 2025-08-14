/**
 * main.c - Kiloeエディタのエントリポイント
 * 
 * プログラムの初期化とメインループを管理
 */

#include "kiloe.h"

/* グローバル変数の定義（他のファイルからexternで参照される） */
struct editorSettings Config;
struct editorSyntax HLDB[] = {
  {
    "c",
    (char*[]){"*.c", "*.h", "*.cpp", NULL},
    (char*[]){
      "switch", "if", "while", "for", "break", "continue", "return", "else",
      "struct", "union", "typedef", "static", "enum", "class", "case",
      "int|", "long|", "double|", "float|", "char|", "unsigned|", "signed|",
      "void|", NULL
    },
    "//", "/*", "*/",
    HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
  },
  // 終端マーカー（新しい言語サポートはここに追加）
  { NULL, NULL, NULL, NULL, NULL, NULL, 0 }
};

/**
 * getHLDBEntries - HLDB配列のエントリ数を動的に計算
 * 
 * 終端マーカー（filetype == NULL）を探して配列サイズを返す
 * これにより新しい言語サポートを追加してもコードの変更が不要
 * 
 * @return: HLDB配列のエントリ数（終端マーカーは除く）
 */
int getHLDBEntries() {
  int count = 0;
  while (HLDB[count].filetype != NULL) {
    count++;
  }
  return count;
}

/**
 * initEditor - エディタの初期化
 * 
 * エディタの状態を初期化し、設定ファイルを読み込み、
 * ウィンドウサイズを取得する
 */
void initEditor() {
  // デフォルト設定を読み込み
  initDefaultConfig();
  
  // 設定ファイルを探索・読み込み
  // 優先順位: ./kiloe.conf -> ~/.kiloe.conf
  if (loadConfig("kiloe.conf") != 0) {
    // カレントディレクトリになければホームディレクトリを探索
    char config_path[256];
    const char *home = getenv("HOME");
    if (home) {
      snprintf(config_path, sizeof(config_path), "%s/.kiloe.conf", home);
      loadConfig(config_path);
    }
  }
  
  // エディタ状態の初期化
  E.cx = 0;              // カーソルX位置
  E.cy = 0;              // カーソルY位置
  E.rx = 0;              // レンダリング時のX位置
  E.rowoff = 0;          // 行スクロールオフセット
  E.coloff = 0;          // 列スクロールオフセット
  E.numrows = 0;         // 総行数
  E.row = NULL;          // 行データ配列
  E.dirty = 0;           // 変更フラグ
  E.filename = NULL;     // ファイル名
  E.statusmsg[0] = '\0'; // ステータスメッセージ
  E.statusmsg_time = 0;  // メッセージ表示時刻
  E.syntax = NULL;       // シンタックスハイライト設定

  // ウィンドウサイズの取得
  if (getWindowSize(&E.screenrows, &E.screencols) == -1) {
    die("getWindowSize");
  }
  // ステータス行とメッセージ行のスペースを確保
  E.screenrows -= 2;
}

/**
 * main - プログラムのエントリポイント
 * @argc: コマンドライン引数の数
 * @argv: コマンドライン引数の配列
 * 
 * エディタを初期化し、ファイルを開き（指定された場合）、
 * メインループを実行する
 * 
 * @return: 常に0（正常終了）
 */
int main(int argc, char *argv[]) {
  // ターミナルをRaw modeに設定
  enableRawMode();
  
  // エディタを初期化
  initEditor();
  
  // コマンドライン引数でファイルが指定されていれば開く
  if (argc >= 2) {
    editorOpen(argv[1]);
  }

  // ヘルプメッセージを表示
  editorSetStatusMessage("HELP: Ctrl-s = save | Ctrl-q = quit | Ctrl-f = find");

  // メインループ：画面更新とキー入力処理を繰り返す
  while (1) {
    editorRefreshScreen();
    editorProcessKeypress();
  }
  
  return 0;
}