/**
 * search.c - 検索機能
 * 
 * テキスト検索とハイライト機能を提供：
 * - インクリメンタルサーチ
 * - 検索結果のハイライト表示
 * - 前方・後方検索
 * - UTF-8文字対応の検索
 */

#include "kiloe.h"

/**
 * 検索コールバック関数
 * ユーザーの入力に応じてリアルタイムで検索を実行
 */
void editorFindCallback(char *query, int key) {
    static int last_match = -1;    // 最後にマッチした行のインデックス
    static int direction = 1;      // 検索方向（1: 順方向、-1: 逆方向）
    
    static int saved_hl_line;      // ハイライト保存用の行番号
    static char *saved_hl = NULL;  // ハイライト保存用バッファ

    // 前回のハイライトを復元
    if (saved_hl) {
        memcpy(E.row[saved_hl_line].hl, saved_hl, E.row[saved_hl_line].rsize);
        free(saved_hl);
        saved_hl = NULL;
    }

    // キー入力による動作制御
    if (key == '\r' || key == ESC) {
        // Enter または ESC で検索終了
        last_match = -1;
        direction = 1;
        return;
    } else if (key == ARROW_RIGHT || key == ARROW_DOWN) {
        // 右矢印・下矢印で次を検索
        direction = 1;
    } else if (key == ARROW_LEFT || key == ARROW_UP) {
        // 左矢印・上矢印で前を検索
        direction = -1;
    } else {
        // 文字入力時は新規検索開始
        last_match = -1;
        direction = 1;
    }

    // 検索開始位置の設定
    if (last_match == -1) direction = 1;
    int current = last_match;

    // 全行を検索
    int i;
    for (i = 0; i < E.numrows; i++) {
        current += direction;
        // 循環検索（最後の行の次は最初の行）
        if (current == -1) current = E.numrows - 1;
        else if (current == E.numrows) current = 0;

        erow *row = &E.row[current];
        
        // UTF-8対応：charsバッファ内で検索
        char *chars_match = strstr(row->chars, query);
        if (chars_match) {
            last_match = current;
            E.cy = current;
            // カーソル位置をマッチ位置に設定（バイト位置）
            E.cx = chars_match - row->chars;
            // 画面をマッチした行まで移動
            E.rowoff = E.numrows;

            // ハイライト表示用：renderバッファでもマッチ位置を検索
            char *render_match = strstr(row->render, query);
            if (render_match) {
                // 現在のハイライト状態を保存
                saved_hl_line = current;
                saved_hl = malloc(row->rsize);
                memcpy(saved_hl, row->hl, row->rsize);

                // 検索結果をハイライト
                memset(&row->hl[render_match - row->render], HL_MATCH, strlen(query));
            }
            break;
        }
    }
}

/**
 * 検索機能の開始
 * プロンプトを表示してユーザーの検索クエリを受け付け
 */
void editorFind() {
    // 検索前の状態を保存（ESCでキャンセルした場合の復元用）
    int saved_cx = E.cx;
    int saved_cy = E.cy;
    int saved_coloff = E.coloff;
    int saved_rowoff = E.rowoff;

    // 検索プロンプトを表示（コールバック付き）
    char *query = editorPrompt("Search: %s (ESC/Arrows/Enter)", editorFindCallback);
    
    if (query) {
        // 検索が実行された場合はメモリを解放
        free(query);
    } else {
        // ESCでキャンセルされた場合はカーソル位置を復元
        E.cx = saved_cx;
        E.cy = saved_cy;
        E.coloff = saved_coloff;
        E.rowoff = saved_rowoff;
    }
}