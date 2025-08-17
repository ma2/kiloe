/**
 * input.c - 入力処理機能
 * 
 * キーボード入力の処理とユーザーインターフェースを提供：
 * - 入力プロンプト表示
 * - カーソル移動処理
 * - キー入力の解釈と実行
 * - UTF-8文字入力対応
 * - 特殊キーの処理
 */

#include "kiloe.h"

/**
 * 入力プロンプトの表示と入力受付
 * コールバック機能付きでリアルタイム処理可能
 */
char *editorPrompt(char *prompt, void (*callback)(char *, int)) {
    size_t bufsize = 128;
    char *buf = malloc(bufsize);
    
    size_t buflen = 0;
    buf[0] = '\0';

    while(1) {
        editorSetStatusMessage(prompt, buf);
        editorRefreshScreen();

        int c = editorReadKey();
        
        if (c == DELETE || c == CTRL_KEY('h') || c == BACKSPACE) {
            // バックスペース処理（UTF-8文字境界を考慮）
            if (buflen != 0) {
                // UTF-8文字の先頭まで戻る
                buflen--;
                while (buflen > 0 && is_utf8_continuation((unsigned char)buf[buflen])) {
                    buflen--;
                }
                buf[buflen] = '\0';
            }
        } else if (c == ESC) {
            // ESCでキャンセル
            editorSetStatusMessage("");
            if (callback) callback(buf, c);
            free(buf);
            return NULL;
        } else if (c == '\r') {
            // Enterで確定
            if (buflen != 0) {
                editorSetStatusMessage("");
                if (callback) callback(buf, c);
                return buf;
            }
        } else if (c == ARROW_UP || c == ARROW_DOWN || c == ARROW_LEFT || c == ARROW_RIGHT) {
            // 矢印キーをコールバックに送信（検索機能で使用）
            if (callback) callback(buf, c);
        } else if (!iscntrl(c) || (unsigned char)c >= 0x80) {
            // 通常文字またはUTF-8文字の入力
            if (buflen == bufsize - 1) {
                // バッファ不足の場合は倍に拡張
                bufsize *= 2;
                buf = realloc(buf, bufsize);
            }
            buf[buflen++] = c;
            buf[buflen] = '\0';
        }

        // コールバック関数を呼び出し
        if (callback) callback(buf, c);
    }
}

/**
 * カーソル移動処理
 * UTF-8文字境界を考慮した適切な移動
 */
void editorMoveCursor(int key) {
    erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];

    switch(key) {
        case ARROW_LEFT:
            if (E.cx != 0) {
                // UTF-8文字境界を考慮して前の文字へ移動
                E.cx = move_to_prev_char(row->chars, E.cx);
            } else if (E.cy > 0) {
                // 行頭で左矢印：前行の行末へ移動
                E.cy--;
                E.cx = E.row[E.cy].size;
            }
            break;
            
        case ARROW_RIGHT:
            if (row && E.cx < row->size) {
                // UTF-8文字境界を考慮して次の文字へ移動
                E.cx = move_to_next_char(row->chars, E.cx, row->size);
            } else if (row && E.cx == row->size) {
                // 行末で右矢印：次行の行頭へ移動
                E.cy++;
                E.cx = 0;
            }
            break;
            
        case ARROW_UP:
        case ARROW_DOWN: {
            // 上下移動時は現在の表示位置(rx)を保持
            int target_rx = E.rx;
            if (E.cy < E.numrows) {
                target_rx = editorRowCxToRx(&E.row[E.cy], E.cx);
            }
            
            // 行を移動
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
    
    // カーソル位置が行の範囲内に収まるよう調整
    row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
    int rowlen = row ? row->size : 0;
    if (E.cx > rowlen) {
        E.cx = rowlen;
    }
}

/**
 * キー入力処理のメイン関数
 * 各種キーに対応する操作を実行
 */
void editorProcessKeypress() {
    static int quit_times = -1;  // 初期化フラグ（-1は未初期化）
    if (quit_times == -1) quit_times = Config.quit_times;
    
    int c = editorReadKey();
    
    switch (c) {
        case '\r':
            // Enterキー：改行挿入
            editorInsertNewLine();
            break;

        case CTRL_KEY('q'):
            // Ctrl+Q：終了
            if (E.dirty && quit_times > 0) {
                editorSetStatusMessage("WARNING!!! File has unsaved changes. "
                    "Press Ctrl-Q %d more times to quit.", quit_times);
                quit_times--;
                return;
            }
            // 画面クリアして終了
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;

        case CTRL_KEY('s'):
            // Ctrl+S：保存
            editorSave();
            break;

        case HOME:
            // Home：行頭に移動
            E.cx = 0;
            break;

        case END:
            // End：行末に移動
            if (E.cy < E.numrows) {
                E.cx = E.row[E.cy].size;
            }
            break;
        
        case CTRL_KEY('f'):
            // Ctrl+F：検索
            editorFind();
            break;

        case BACKSPACE:
        case CTRL_KEY('h'):
        case DELETE:
            // バックスペース・削除
            if (c == DELETE) editorMoveCursor(ARROW_RIGHT);
            editorDelChar();
            break;

        case PAGE_UP:
        case PAGE_DOWN:
            {
                // ページアップ・ダウン
                if (c == PAGE_UP) {
                    E.cy = E.rowoff;
                } else if (c == PAGE_DOWN) {
                    E.cy = E.rowoff + E.screenrows - 1;
                    if (E.cy > E.numrows) E.cy = E.numrows;
                }

                // 画面分だけカーソルを移動
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
            // 矢印キー：カーソル移動
            editorMoveCursor(c);
            break;

        case CTRL_KEY('l'):
        case ESC:
            // 画面リフレッシュ・ESC（何もしない）
            break;

        default:
            // その他：文字挿入
            editorInsertChar(c);
            break;
    }

    // 終了確認回数をリセット
    quit_times = Config.quit_times;
}