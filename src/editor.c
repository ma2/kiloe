/**
 * editor.c - エディタ操作機能
 * 
 * テキストエディタの基本編集操作を提供：
 * - 文字の挿入
 * - 改行の挿入
 * - 文字の削除（UTF-8対応）
 * 
 * UTF-8マルチバイト文字を適切に処理し、文字境界を考慮した削除を行う
 */

#include "kiloe.h"

/**
 * カーソル位置に文字を挿入
 * 必要に応じて新しい行を作成
 */
void editorInsertChar(int c) {
    // ファイル末尾（最終行の次）にいる場合は新しい行を作成
    if (E.cy == E.numrows) {
        editorInsertRow(E.numrows, "", 0);
    }
    
    // 現在行のカーソル位置に文字を挿入
    editorRowInsertChar(&E.row[E.cy], E.cx, c);
    // カーソルを1つ右に移動
    E.cx++;
}

/**
 * カーソル位置で改行を挿入
 * 行の分割処理も含む
 */
void editorInsertNewLine() {
    if (E.cx == 0) {
        // 行頭の場合：現在行の前に空行を挿入
        editorInsertRow(E.cy, "", 0);
    } else {
        // 行の途中の場合：現在行を分割
        erow *row = &E.row[E.cy];
        
        // カーソル位置以降の文字で新しい行を作成
        editorInsertRow(E.cy + 1, &row->chars[E.cx], row->size - E.cx);
        
        // 現在行をカーソル位置で切断
        row = &E.row[E.cy];  // reallocで位置が変わる可能性があるので再取得
        row->size = E.cx;
        row->chars[row->size] = '\0';
        editorUpdateRow(row);
    }
    
    // カーソルを次行の行頭に移動
    E.cy++;
    E.cx = 0;
}

/**
 * カーソル直前の文字を削除（バックスペース）
 * UTF-8マルチバイト文字を適切に処理
 */
void editorDelChar() {
    // ファイル末尾（最終行の次）にいる場合は何もしない
    if (E.cy == E.numrows) return;
    // ファイル先頭（最初の行の行頭）にいる場合は何もしない
    if (E.cx == 0 && E.cy == 0) return;

    erow *row = &E.row[E.cy];
    
    if (E.cx > 0) {
        // 行の途中の場合：文字を削除
        
        // UTF-8文字の開始位置を取得
        int prev_pos = move_to_prev_char(row->chars, E.cx);
        int delete_bytes = E.cx - prev_pos;
        
        // マルチバイト文字の全バイトを削除
        for (int i = 0; i < delete_bytes; i++) {
            editorRowDelChar(row, prev_pos);
        }
        
        // カーソルを削除した文字の開始位置に移動
        E.cx = prev_pos;
    } else {
        // 行頭の場合：現在行を前の行に結合
        
        // 前の行の末尾にカーソルを移動
        E.cx = E.row[E.cy - 1].size;
        // 現在行の内容を前の行に追加
        editorRowAppendString(&E.row[E.cy - 1], row->chars, row->size);
        // 現在行を削除
        editorDelRow(E.cy);
        // カーソルを前の行に移動
        E.cy--;
    }
}