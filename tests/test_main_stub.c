/**
 * test_main_stub.c - テスト用のmain.cの代替
 * 
 * main関数を除いたグローバル変数とHLDB定義のみを提供
 */

#include "../src/kiloe.h"

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
  // 終端マーカー
  { NULL, NULL, NULL, NULL, NULL, NULL, 0 }
};

/**
 * getHLDBEntries - HLDB配列のエントリ数を動的に計算
 */
int getHLDBEntries() {
    int count = 0;
    while (HLDB[count].filetype != NULL) {
        count++;
    }
    return count;
}