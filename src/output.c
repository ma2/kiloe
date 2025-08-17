/**
 * output.c - 画面出力機能
 * 
 * エディタの画面描画とレンダリング機能を提供：
 * - スクロール処理
 * - テキスト行の描画
 * - ステータスバーの描画
 * - メッセージバーの描画
 * - 画面全体の更新
 * - シンタックスハイライト表示
 * - UTF-8文字の適切な表示
 */

#include "kiloe.h"

/**
 * 行番号表示幅を計算
 */
static int getLineNumberWidth() {
    if (!Config.show_line_numbers || E.numrows == 0) return 0;
    
    int width = 1;  // 最低1桁
    int num = E.numrows;
    while (num >= 10) {
        width++;
        num /= 10;
    }
    return width + 1;  // +1はスペース用
}

/**
 * スクロール処理
 * カーソル位置に応じて表示領域を調整
 */
void editorScroll() {
    // 現在行のカーソル位置を表示位置に変換
    E.rx = E.cx;
    if (E.cy < E.numrows) {
        E.rx = editorRowCxToRx(&E.row[E.cy], E.cx);
    }
    
    // 垂直スクロール処理
    if (E.cy < E.rowoff) {
        // カーソルが表示領域の上にある場合
        E.rowoff = E.cy;
    }
    if (E.cy >= E.rowoff + E.screenrows) {
        // カーソルが表示領域の下にある場合
        E.rowoff = E.cy - E.screenrows + 1;
    }
    
    // 水平スクロール処理（行番号幅を考慮）
    int line_num_width = getLineNumberWidth();
    int effective_cols = E.screencols - line_num_width;
    
    if (E.rx < E.coloff) {
        // カーソルが表示領域の左にある場合
        E.coloff = E.rx;
    }
    if (E.rx >= E.coloff + effective_cols) {
        // カーソルが表示領域の右にある場合
        E.coloff = E.rx - effective_cols + 1;
    }
}

/**
 * テキスト行の描画
 * シンタックスハイライトとUTF-8文字を適切に処理
 */
void editorDrawRows(struct abuf *ab) {
    int y;
    int line_num_width = getLineNumberWidth();  // 行番号幅を計算
    
    for (y = 0; y < E.screenrows; y++) {
        int filerow = y + E.rowoff;
        
        if (filerow >= E.numrows) {
            // 行番号表示が有効な場合はスペースを確保
            if (line_num_width > 0) {
                for (int i = 0; i < line_num_width; i++) {
                    abAppend(ab, " ", 1);
                }
            }
            
            // ファイル末尾を超えた行の処理
            if (E.numrows == 0 && y == E.screenrows / 3) {
                // 空ファイルの場合はウェルカムメッセージを表示
                char welcome[80];
                int welcomelen = snprintf(welcome, sizeof(welcome), "%s", Config.welcome_message);
                if (welcomelen > E.screencols) welcomelen = E.screencols;
                
                // 画面中央に配置
                int padding = (E.screencols - welcomelen) / 2;
                if (padding) {
                    abAppend(ab, "~", 1);
                    padding--;
                }
                while (padding--) abAppend(ab, " ", 1);
                abAppend(ab, welcome, welcomelen);
            } else {
                // 通常は各行にチルダを表示
                abAppend(ab, "~", 1);
            }
        } else {
            // 行番号を表示
            if (line_num_width > 0) {
                char line_num[16];
                int num_len = snprintf(line_num, sizeof(line_num), "%*d ", 
                                     line_num_width - 1, filerow + 1);
                abAppend(ab, line_num, num_len);
            }
            
            // 実際のテキスト行の描画
            int len = E.row[filerow].rsize - E.coloff;
            if (len < 0) len = 0;
            // テキスト表示領域を行番号分だけ狭める
            int text_cols = E.screencols - line_num_width;
            if (len > text_cols) len = text_cols;
            
            char *c = &E.row[filerow].render[E.coloff];
            unsigned char *hl = &E.row[filerow].hl[E.coloff];
            int current_color = -1;
            
            // 1文字ずつ描画（色付きで）
            for (int j = 0; j < len; j++) {
                // UTF-8制御文字チェック（0x80以上は制御文字でない）
                if ((unsigned char)c[j] < 0x80 && iscntrl(c[j])) {
                    // 制御文字の場合は反転表示
                    char sym = (c[j] <= 26) ? '@' + c[j] : '?';
                    abAppend(ab, "\x1b[7m", 4);    // 反転開始
                    abAppend(ab, &sym, 1);
                    abAppend(ab, "\x1b[m", 3);     // リセット
                    
                    // 色が設定されている場合は復元
                    if (current_color != -1) {
                        char buf[16];
                        int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", current_color);
                        abAppend(ab, buf, clen);
                    }
                } else if (hl[j] == HL_NORMAL) {
                    // 通常文字の場合
                    if (current_color != -1) {
                        abAppend(ab, "\x1b[39m", 5);  // デフォルト色にリセット
                        current_color = -1;
                    }
                    abAppend(ab, &c[j], 1);
                } else {
                    // ハイライト文字の場合
                    int color = editorSyntaxToColor(hl[j]);
                    if (color != current_color) {
                        // 色が変わった場合のみ色変更コマンドを送信
                        current_color = color;
                        char buf[16];
                        int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", color);
                        abAppend(ab, buf, clen);
                    }
                    abAppend(ab, &c[j], 1);
                }
            }
            // 行末で色をリセット
            abAppend(ab, "\x1b[39m", 5);
        }
        
        // 行末クリアと改行
        abAppend(ab, "\x1b[K", 3);  // 行末まで削除
        abAppend(ab, "\r\n", 2);    // 改行
    }
}

/**
 * ステータスバーの描画
 * ファイル名、行数、変更状態、カーソル位置等を表示
 */
void editorDrawStatusBar(struct abuf *ab) {
    abAppend(ab, "\x1b[7m", 4);  // 反転表示開始
    
    char status[80];   // 左側のステータス情報
    char rstatus[80];  // 右側のステータス情報
    
    // 左側：ファイル名と行数、変更状態
    int len = snprintf(status, sizeof(status), "%.20s - %d lines %s", 
        E.filename ? E.filename : "[No Name]", 
        E.numrows, 
        E.dirty ? "(modified)" : "");
    
    // 右側：ファイルタイプと現在位置
    int rlen = snprintf(rstatus, sizeof(rstatus), "%s | %d/%d", 
        E.syntax ? E.syntax->filetype : "no ft", 
        E.cy + 1, E.numrows);
    
    if (len > E.screencols) len = E.screencols;
    abAppend(ab, status, len);
    
    // 左右のステータスの間を埋める
    while (len < E.screencols) {
        if (E.screencols - len == rlen) {
            abAppend(ab, rstatus, rlen);
            break;
        } else {
            abAppend(ab, " ", 1);
            len++;
        }
    }
    
    abAppend(ab, "\x1b[m", 3);   // 反転表示終了
    abAppend(ab, "\r\n", 2);     // 改行
}

/**
 * メッセージバーの描画
 * 一時的なメッセージを表示（設定された時間後に消える）
 */
void editorDrawMessageBar(struct abuf *ab) {
    abAppend(ab, "\x1b[K", 3);   // 行クリア
    
    int msglen = strlen(E.statusmsg);
    if (msglen > E.screencols) msglen = E.screencols;
    
    // メッセージが設定されており、タイムアウト内の場合のみ表示
    if (msglen && time(NULL) - E.statusmsg_time < Config.status_timeout) {
        abAppend(ab, E.statusmsg, msglen);
    }
}

/**
 * 画面全体の更新
 * 全てのコンポーネントを描画してターミナルに出力
 */
void editorRefreshScreen() {
    editorScroll();  // スクロール処理

    struct abuf ab = ABUF_INIT;

    // カーソル非表示
    abAppend(&ab, "\x1b[?25l", 6);
    // カーソルをホーム位置に移動
    abAppend(&ab, "\x1b[H", 3);

    // 各コンポーネントの描画
    editorDrawRows(&ab);
    editorDrawStatusBar(&ab);
    editorDrawMessageBar(&ab);

    // カーソルを正しい位置に移動（行番号幅を考慮）
    int line_num_width = getLineNumberWidth();
    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", 
        E.cy - E.rowoff + 1, 
        E.rx - E.coloff + 1 + line_num_width);  // 行番号幅分右にシフト
    abAppend(&ab, buf, strlen(buf));

    // カーソル表示
    abAppend(&ab, "\x1b[?25h", 6);

    // バッファの内容を一度にターミナルに出力
    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}

/**
 * ステータスメッセージの設定
 * printfスタイルでメッセージを設定し、現在時刻を記録
 */
void editorSetStatusMessage(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
    va_end(ap);
    E.statusmsg_time = time(NULL);
}