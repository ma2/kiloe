/**
 * row.c - 行操作機能
 * 
 * テキストの各行に対する基本操作を提供：
 * - カーソル位置変換（文字位置⇔表示位置）
 * - 行の更新・挿入・削除
 * - 文字の挿入・削除・追加
 * - UTF-8とタブ文字の適切な処理
 */

#include "kiloe.h"

/**
 * chars内カーソル位置をrender内カーソル位置に変換
 * UTF-8文字の表示幅とタブ展開を考慮した位置計算
 */
int editorRowCxToRx(erow *row, int cx) {
    int rx = 0;
    int j = 0;
    
    while (j < cx && j < row->size) {
        if (row->chars[j] == '\t') {
            // タブ位置の計算（次のタブストップ位置まで）
            rx += (KILO_TAB_STOP - 1) - (rx % KILO_TAB_STOP);
            rx++;
            j++;
        } else {
            // UTF-8文字の表示幅を取得（日本語文字は2幅）
            int char_width = get_char_width(row->chars, j);
            rx += char_width;
            // 次のUTF-8文字の開始位置へ移動
            j = move_to_next_char(row->chars, j, row->size);
        }
    }
    return rx;
}

/**
 * render内カーソル位置をchars内カーソル位置に変換
 * 表示位置から実際のバイト位置を逆算
 */
int editorRowRxToCx(erow *row, int rx) {
    int cur_rx = 0;
    int cx = 0;
    
    while (cx < row->size) {
        if (row->chars[cx] == '\t') {
            // タブの表示幅を計算
            cur_rx += (KILO_TAB_STOP - 1) - (cur_rx % KILO_TAB_STOP);
            cur_rx++;
            // 指定表示位置を超えた場合は現在位置を返す
            if (cur_rx > rx) return cx;
            cx++;
        } else {
            // UTF-8文字の表示幅を取得
            int char_width = get_char_width(row->chars, cx);
            // 指定表示位置を超える場合は現在位置を返す
            if (cur_rx + char_width > rx) return cx;
            cur_rx += char_width;
            // 次のUTF-8文字の開始位置へ移動
            cx = move_to_next_char(row->chars, cx, row->size);
        }
    }
    return cx;
}

/**
 * 行の表示用データを更新
 * タブをスペースに展開してrenderバッファを構築し、シンタックスハイライトも更新
 */
void editorUpdateRow(erow *row) {
    // タブ文字の数をカウント
    int tabs = 0;
    int j;
    for (j = 0; j < row->size; j++) {
        if (row->chars[j] == '\t') tabs++;
    }

    // renderバッファを再割り当て（タブ展開分を考慮）
    free(row->render);
    row->render = malloc(row->size + tabs * (KILO_TAB_STOP - 1) + 1);

    // charsからrenderへのコピー（タブをスペースに展開）
    int idx = 0;          // renderバイト位置
    int display_col = 0;  // 表示列位置
    j = 0;
    
    while (j < row->size) {
        if (row->chars[j] == '\t') {
            // タブを適切な数のスペースに展開
            row->render[idx++] = SPC;
            display_col++;
            // 次のタブストップ位置まで空白を追加
            while (display_col % KILO_TAB_STOP != 0) {
                row->render[idx++] = SPC;
                display_col++;
            }
            j++;
        } else {
            // UTF-8文字の処理
            int char_width = get_char_width(row->chars, j);
            int bytes = utf8_char_len(row->chars[j]);
            
            // 文字のバイトをコピー
            for (int k = 0; k < bytes && j + k < row->size; k++) {
                row->render[idx++] = row->chars[j + k];
            }
            
            display_col += char_width;
            j += bytes;
        }
    }
    
    row->render[idx] = '\0';
    row->rsize = idx;

    // シンタックスハイライト情報を更新
    editorUpdateSyntax(row);
}

/**
 * 指定位置に新しい行を挿入
 */
void editorInsertRow(int at, char *s, size_t len) {
    // 挿入位置の妥当性チェック
    if (at < 0 || at > E.numrows) return;

    // 行配列を1つ拡張
    E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
    // 挿入位置以降の行を後ろにシフト
    memmove(&E.row[at + 1], &E.row[at], sizeof(erow) * (E.numrows - at));
    
    // シフト後の行インデックスを更新
    for (int j = at + 1; j <= E.numrows; j++) E.row[j].idx++;

    // 新しい行を初期化
    E.row[at].idx = at;
    E.row[at].size = len;
    E.row[at].chars = malloc(len + 1);
    memcpy(E.row[at].chars, s, len);
    E.row[at].chars[len] = '\0';
    
    // 表示用データを初期化
    E.row[at].rsize = 0;
    E.row[at].render = NULL;
    E.row[at].hl = NULL;
    E.row[at].hl_open_comment = 0;
    
    // 行データを更新
    editorUpdateRow(&E.row[at]);
    
    // 行数とダーティフラグを更新
    E.numrows++;
    E.dirty++;
}

/**
 * 行のメモリを解放
 */
void editorFreeRow(erow *row) {
    free(row->render);
    free(row->chars);
    free(row->hl);
}

/**
 * 指定位置の行を削除
 */
void editorDelRow(int at) {
    // 削除位置の妥当性チェック
    if (at < 0 || at >= E.numrows) return;
    
    // 行のメモリを解放
    editorFreeRow(&E.row[at]);
    // 削除位置以降の行を前にシフト
    memmove(&E.row[at], &E.row[at + 1], sizeof(erow) * (E.numrows - at - 1));
    
    // シフト後の行インデックスを更新
    for (int j = at; j < E.numrows - 1; j++) E.row[j].idx = j;
    
    // 行数とダーティフラグを更新
    E.numrows--;
    E.dirty++;
}

/**
 * 行の指定位置に文字を挿入
 */
void editorRowInsertChar(erow *row, int at, int c) {
    // 挿入位置の調整（範囲外なら行末に）
    if (at < 0 || at > row->size) at = row->size;
    
    // 文字配列を1バイト拡張
    row->chars = realloc(row->chars, row->size + 2);
    // 挿入位置以降の文字を後ろにシフト
    memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
    
    // 新しい文字を挿入
    row->size++;
    row->chars[at] = c;
    
    // 行データを更新
    editorUpdateRow(row);
    E.dirty++;
}

/**
 * 行末尾に文字列を追加
 */
void editorRowAppendString(erow *row, char *s, size_t len) {
    // 文字配列を必要な分だけ拡張
    row->chars = realloc(row->chars, row->size + len + 1);
    // 文字列を行末に追加
    memcpy(&row->chars[row->size], s, len);
    row->size += len;
    row->chars[row->size] = '\0';
    
    // 行データを更新
    editorUpdateRow(row);
    E.dirty++;
}

/**
 * 行の指定位置の文字を削除
 */
void editorRowDelChar(erow *row, int at) {
    // 削除位置の妥当性チェック
    if (at < 0 || at >= row->size) return;
    
    // 削除位置以降の文字を前にシフト
    memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
    row->size--;
    
    // 行データを更新
    editorUpdateRow(row);
    E.dirty++;
}