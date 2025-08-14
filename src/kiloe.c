/**
 * kiloe.c - Kiloeエディタの残り機能
 * 
 * Phase 1で分割されていない機能群：
 * - 設定管理
 * - シンタックスハイライト
 * - バッファ操作
 * - エディタ操作
 * - ファイルI/O
 * - 検索機能  
 * - 画面描画
 * - 入力処理
 */

#include "kiloe.h"

/* ここから、Phase1で分割されていない機能の実装 */

/* Phase1で分割していない機能の実装開始 */

/** settings */

// デフォルト設定を初期化
void initDefaultConfig() {
    // エディタ設定
    Config.tab_stop = 8;
    Config.quit_times = 3;
    Config.show_line_numbers = 0;
    
    // 表示設定
    strcpy(Config.welcome_message, "Kilo editor -- version 0.0.1");
    Config.status_timeout = 5;
    
    // カラー設定
    Config.color_comment = 36;      // シアン
    Config.color_keyword1 = 33;     // 黄色
    Config.color_keyword2 = 32;     // 緑
    Config.color_string = 35;       // マゼンタ
    Config.color_number = 31;       // 赤
    Config.color_match = 34;        // 青
}

// 文字列をtrueまたはfalseとして解釈
int parseBool(const char *value) {
    if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0) {
        return 1;
    }
    return 0;
}

// 設定ファイルを読み込み
int loadConfig(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return -1;
    
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        // コメント行と空行をスキップ
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        
        // = で分割
        char *eq = strchr(line, '=');
        if (!eq) continue;
        
        *eq = '\0';
        char *key = line;
        char *value = eq + 1;
        
        // 改行文字を削除
        char *newline = strchr(value, '\n');
        if (newline) *newline = '\0';
        newline = strchr(value, '\r');
        if (newline) *newline = '\0';
        
        // 先頭と末尾の空白を削除
        while (*key == ' ' || *key == '\t') key++;
        while (*value == ' ' || *value == '\t') value++;
        
        // 設定値を適用
        if (strcmp(key, "tab_stop") == 0) {
            Config.tab_stop = atoi(value);
        } else if (strcmp(key, "quit_times") == 0) {
            Config.quit_times = atoi(value);
        } else if (strcmp(key, "show_line_numbers") == 0) {
            Config.show_line_numbers = parseBool(value);
        } else if (strcmp(key, "welcome_message") == 0) {
            strncpy(Config.welcome_message, value, sizeof(Config.welcome_message) - 1);
            Config.welcome_message[sizeof(Config.welcome_message) - 1] = '\0';
        } else if (strcmp(key, "status_timeout") == 0) {
            Config.status_timeout = atoi(value);
        } else if (strcmp(key, "color_comment") == 0) {
            Config.color_comment = atoi(value);
        } else if (strcmp(key, "color_keyword1") == 0) {
            Config.color_keyword1 = atoi(value);
        } else if (strcmp(key, "color_keyword2") == 0) {
            Config.color_keyword2 = atoi(value);
        } else if (strcmp(key, "color_string") == 0) {
            Config.color_string = atoi(value);
        } else if (strcmp(key, "color_number") == 0) {
            Config.color_number = atoi(value);
        } else if (strcmp(key, "color_match") == 0) {
            Config.color_match = atoi(value);
        }
    }
    
    fclose(fp);
    return 0;
}

/* UTF-8機能はutf8.cに移動済み */

/* Terminal機能はterminal.cに移動済み */

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
    case HL_MLCOMMENT: return Config.color_comment;
    case HL_KEYWORD1: return Config.color_keyword1;
    case HL_KEYWORD2: return Config.color_keyword2;
    case HL_STRING: return Config.color_string;
    case HL_NUMBER: return Config.color_number;
    case HL_MATCH: return Config.color_match;
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
    // UTF-8文字境界を考慮して削除
    int prev_pos = move_to_prev_char(row->chars, E.cx);
    int delete_bytes = E.cx - prev_pos;
    
    // マルチバイト文字全体を削除
    for (int i = 0; i < delete_bytes; i++) {
      editorRowDelChar(row, prev_pos);
    }
    E.cx = prev_pos;
  } else {
    // 行頭でのBS→現在行を前行の後ろにアペンドする
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
    
    // charsバッファ内で検索（マルチバイト文字対応）
    char *chars_match = strstr(row->chars, query);
    if (chars_match) {
      last_match = current;
      E.cy = current;
      // charsバッファ内での位置をそのまま使用
      E.cx = chars_match - row->chars;
      // マッチした行を先頭にする
      E.rowoff = E.numrows;

      // ハイライト表示のためのrenderバッファ内位置を計算
      char *render_match = strstr(row->render, query);
      if (render_match) {
        // ハイライト前の状態を保存
        saved_hl_line = current;
        saved_hl = malloc(row->rsize);
        memcpy(saved_hl, row->hl, row->rsize);

        // findした文字列をハイライト
        memset(&row->hl[render_match - row->render], HL_MATCH, strlen(query));
      }
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

void abAppend(struct abuf *ab, const char *s, int len) {
  char *new = realloc(ab->b, ab->len + len);
  if (new == NULL) return;
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
        int welcomelen = snprintf(welcome, sizeof(welcome), "%s", Config.welcome_message);
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
  if (msglen && time(NULL) - E.statusmsg_time < Config.status_timeout) {
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
      // バッファ内でBSを有効にする（UTF-8文字境界を考慮）
      if (buflen != 0) {
        // UTF-8文字の先頭を探す
        buflen--;
        while (buflen > 0 && is_utf8_continuation((unsigned char)buf[buflen])) {
          buflen--;
        }
        buf[buflen] = '\0';
      }
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
    } else if (c == ARROW_UP || c == ARROW_DOWN || c == ARROW_LEFT || c == ARROW_RIGHT) {
      // カーソルキーをコールバックに渡す（検索で使用）
      if (callback) callback(buf, c);
    } else if (!iscntrl(c) || (unsigned char)c >= 0x80) {
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
  static int quit_times = -1;  // 未初期化を示すため-1を使用
  if (quit_times == -1) quit_times = Config.quit_times;
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

  quit_times = Config.quit_times;
}

/* main関数とinitEditor関数はmain.cに移動済み */