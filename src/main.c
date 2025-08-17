/**
 * main.c - Kiloeエディタのエントリポイント
 * 
 * プログラムの初期化とメインループを管理
 */

#include "kiloe.h"

/* グローバル変数の定義（他のファイルからexternで参照される） */
struct editorConfig E;
struct editorSettings Config;
/* ファイルマッチパターンを定義 */
static char *C_HL_extensions[] = {".c", ".h", ".cpp", ".cc", ".cxx", NULL};
static char *C_HL_keywords[] = {
  "switch", "if", "while", "for", "break", "continue", "return", "else",
  "struct", "union", "typedef", "static", "enum", "class", "case",
  "int|", "long|", "double|", "float|", "char|", "unsigned|", "signed|",
  "void|", NULL
};

static char *Python_HL_extensions[] = {".py", ".pyw", NULL};
static char *Python_HL_keywords[] = {
  "and", "as", "assert", "break", "class", "continue", "def", "del", "elif",
  "else", "except", "False", "finally", "for", "from", "global", "if",
  "import", "in", "is", "lambda", "None", "not", "or", "pass", "raise",
  "return", "True", "try", "while", "with", "yield",
  "int|", "float|", "str|", "list|", "dict|", "tuple|", "set|", "bool|", NULL
};

static char *JavaScript_HL_extensions[] = {".js", ".jsx", ".ts", ".tsx", NULL};
static char *JavaScript_HL_keywords[] = {
  "break", "case", "catch", "class", "const", "continue", "debugger", "default",
  "delete", "do", "else", "export", "extends", "false", "finally", "for",
  "function", "if", "import", "in", "instanceof", "new", "null", "return",
  "super", "switch", "this", "throw", "true", "try", "typeof", "var", "void",
  "while", "with", "yield", "let", "static", "enum", "implements", "package",
  "protected", "interface", "private", "public",
  "boolean|", "number|", "string|", "object|", "undefined|", NULL
};

static char *Ruby_HL_extensions[] = {".rb", ".rbw", ".rake", NULL};
static char *Ruby_HL_keywords[] = {
  "alias", "and", "begin", "break", "case", "class", "def", "defined", "do",
  "else", "elsif", "end", "ensure", "false", "for", "if", "in", "module",
  "next", "nil", "not", "or", "redo", "rescue", "retry", "return", "self",
  "super", "then", "true", "undef", "unless", "until", "when", "while", "yield",
  "Array|", "Hash|", "String|", "Integer|", "Float|", "Symbol|", "Object|", NULL
};

static char *HTML_HL_extensions[] = {".html", ".htm", ".xml", ".xhtml", NULL};
static char *HTML_HL_keywords[] = {
  "html", "head", "title", "body", "div", "span", "p", "a", "img", "br", "hr",
  "h1", "h2", "h3", "h4", "h5", "h6", "ul", "ol", "li", "table", "tr", "td",
  "th", "form", "input", "button", "script", "style", "link", "meta", NULL
};

static char *Markdown_HL_extensions[] = {".md", ".markdown", ".mkd", NULL};
static char *Markdown_HL_keywords[] = { NULL };

struct editorSyntax HLDB[] = {
  {
    "c",
    C_HL_extensions,
    C_HL_keywords,
    "//", "/*", "*/",
    HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
  },
  {
    "python",
    Python_HL_extensions,
    Python_HL_keywords,
    "#", NULL, NULL,
    HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
  },
  {
    "javascript",
    JavaScript_HL_extensions,
    JavaScript_HL_keywords,
    "//", "/*", "*/",
    HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
  },
  {
    "ruby",
    Ruby_HL_extensions,
    Ruby_HL_keywords,
    "#", NULL, NULL,
    HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
  },
  {
    "html",
    HTML_HL_extensions,
    HTML_HL_keywords,
    NULL, "<!--", "-->",
    HL_HIGHLIGHT_STRINGS
  },
  {
    "markdown",
    Markdown_HL_extensions,
    Markdown_HL_keywords,
    NULL, NULL, NULL,
    0
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