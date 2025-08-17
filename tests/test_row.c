/**
 * test_row.c - 行操作関数のテスト
 */

#define _GNU_SOURCE
#include "minunit.h"
#include "../src/kiloe.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 外部変数 */
extern struct editorConfig E;
extern struct editorSettings Config;

/* テスト用のセットアップ */
static void setup_editor() {
    memset(&E, 0, sizeof(E));
    E.numrows = 0;
    E.row = NULL;
    E.dirty = 0;
    E.filename = NULL;
    E.syntax = NULL;
    
    // デフォルト設定
    Config.tab_stop = 8;
}

/* テスト用のクリーンアップ */
static void cleanup_editor() {
    for (int i = 0; i < E.numrows; i++) {
        editorFreeRow(&E.row[i]);
    }
    free(E.row);
    free(E.filename);
    E.row = NULL;
    E.numrows = 0;
}

/* editorRowCxToRxのテスト - ASCII文字のみ */
void test_editorRowCxToRx_ascii() {
    setup_editor();
    
    editorInsertRow(0, "Hello World", 11);
    erow *row = &E.row[0];
    
    TEST_ASSERT_EQ_INT(0, editorRowCxToRx(row, 0));   // 先頭
    TEST_ASSERT_EQ_INT(5, editorRowCxToRx(row, 5));   // "Hello"の後
    TEST_ASSERT_EQ_INT(11, editorRowCxToRx(row, 11)); // 末尾
    
    cleanup_editor();
}

/* editorRowCxToRxのテスト - タブ文字 */
void test_editorRowCxToRx_with_tabs() {
    setup_editor();
    
    editorInsertRow(0, "\tHello\tWorld", 12);
    erow *row = &E.row[0];
    
    // タブは8列に展開される（デフォルト）
    TEST_ASSERT_EQ_INT(0, editorRowCxToRx(row, 0));   // 先頭
    TEST_ASSERT_EQ_INT(8, editorRowCxToRx(row, 1));   // 最初のタブの後
    TEST_ASSERT_EQ_INT(13, editorRowCxToRx(row, 6));  // "Hello"の後
    TEST_ASSERT_EQ_INT(16, editorRowCxToRx(row, 7));  // 2番目のタブの後（位置13から次のタブストップ16）
    
    cleanup_editor();
}

/* editorRowCxToRxのテスト - 日本語文字 */
void test_editorRowCxToRx_multibyte() {
    setup_editor();
    
    editorInsertRow(0, "あいう", 9);  // 各文字3バイト
    erow *row = &E.row[0];
    
    TEST_ASSERT_EQ_INT(0, editorRowCxToRx(row, 0));  // 先頭
    TEST_ASSERT_EQ_INT(2, editorRowCxToRx(row, 3));  // "あ"の後（表示幅2）
    TEST_ASSERT_EQ_INT(4, editorRowCxToRx(row, 6));  // "あい"の後（表示幅4）
    TEST_ASSERT_EQ_INT(6, editorRowCxToRx(row, 9));  // "あいう"の後（表示幅6）
    
    cleanup_editor();
}

/* editorRowRxToCxのテスト - ASCII文字 */
void test_editorRowRxToCx_ascii() {
    setup_editor();
    
    editorInsertRow(0, "Hello World", 11);
    erow *row = &E.row[0];
    
    TEST_ASSERT_EQ_INT(0, editorRowRxToCx(row, 0));   // 先頭
    TEST_ASSERT_EQ_INT(5, editorRowRxToCx(row, 5));   // 表示位置5
    TEST_ASSERT_EQ_INT(11, editorRowRxToCx(row, 11)); // 末尾
    TEST_ASSERT_EQ_INT(11, editorRowRxToCx(row, 20)); // 範囲外
    
    cleanup_editor();
}

/* editorUpdateRowのテスト - タブ展開 */
void test_editorUpdateRow_tabs() {
    setup_editor();
    
    editorInsertRow(0, "a\tb\tc", 5);
    erow *row = &E.row[0];
    
    // renderバッファが正しく作成されているか確認
    TEST_ASSERT_NOT_NULL(row->render);
    TEST_ASSERT_TRUE(row->rsize > row->size);  // タブが展開されている
    
    // タブがスペースに展開されているか確認
    TEST_ASSERT("First char should be 'a'", row->render[0] == 'a');
    TEST_ASSERT("Tab should be expanded to spaces", row->render[1] == ' ');
    
    cleanup_editor();
}

/* editorInsertRowのテスト */
void test_editorInsertRow() {
    setup_editor();
    
    // 最初の行を挿入
    editorInsertRow(0, "First line", 10);
    TEST_ASSERT_EQ_INT(1, E.numrows);
    TEST_ASSERT_STR_EQ("First line", E.row[0].chars);
    TEST_ASSERT_EQ_INT(10, E.row[0].size);
    
    // 先頭に挿入
    editorInsertRow(0, "New first", 9);
    TEST_ASSERT_EQ_INT(2, E.numrows);
    TEST_ASSERT_STR_EQ("New first", E.row[0].chars);
    TEST_ASSERT_STR_EQ("First line", E.row[1].chars);
    
    // 末尾に挿入
    editorInsertRow(2, "Last line", 9);
    TEST_ASSERT_EQ_INT(3, E.numrows);
    TEST_ASSERT_STR_EQ("Last line", E.row[2].chars);
    
    cleanup_editor();
}

/* editorDelRowのテスト */
void test_editorDelRow() {
    setup_editor();
    
    // 3行追加
    editorInsertRow(0, "Line 1", 6);
    editorInsertRow(1, "Line 2", 6);
    editorInsertRow(2, "Line 3", 6);
    TEST_ASSERT_EQ_INT(3, E.numrows);
    
    // 中間の行を削除
    editorDelRow(1);
    TEST_ASSERT_EQ_INT(2, E.numrows);
    TEST_ASSERT_STR_EQ("Line 1", E.row[0].chars);
    TEST_ASSERT_STR_EQ("Line 3", E.row[1].chars);
    
    // 先頭の行を削除
    editorDelRow(0);
    TEST_ASSERT_EQ_INT(1, E.numrows);
    TEST_ASSERT_STR_EQ("Line 3", E.row[0].chars);
    
    cleanup_editor();
}

/* editorRowInsertCharのテスト */
void test_editorRowInsertChar() {
    setup_editor();
    
    editorInsertRow(0, "Hello", 5);
    erow *row = &E.row[0];
    
    // 末尾に文字を挿入
    editorRowInsertChar(row, 5, '!');
    TEST_ASSERT_EQ_INT(6, row->size);
    TEST_ASSERT_STR_EQ("Hello!", row->chars);
    
    // 先頭に文字を挿入
    editorRowInsertChar(row, 0, '>');
    TEST_ASSERT_EQ_INT(7, row->size);
    TEST_ASSERT_STR_EQ(">Hello!", row->chars);
    
    // 中間に文字を挿入
    editorRowInsertChar(row, 1, '<');
    TEST_ASSERT_EQ_INT(8, row->size);
    TEST_ASSERT_STR_EQ("><Hello!", row->chars);
    
    cleanup_editor();
}

/* editorRowDelCharのテスト */
void test_editorRowDelChar() {
    setup_editor();
    
    editorInsertRow(0, "Hello!", 6);
    erow *row = &E.row[0];
    
    // 末尾の文字を削除
    editorRowDelChar(row, 5);
    TEST_ASSERT_EQ_INT(5, row->size);
    TEST_ASSERT_STR_EQ("Hello", row->chars);
    
    // 先頭の文字を削除
    editorRowDelChar(row, 0);
    TEST_ASSERT_EQ_INT(4, row->size);
    TEST_ASSERT_STR_EQ("ello", row->chars);
    
    // 範囲外の削除（何も起こらない）
    editorRowDelChar(row, 10);
    TEST_ASSERT_EQ_INT(4, row->size);
    
    cleanup_editor();
}

/* editorRowAppendStringのテスト */
void test_editorRowAppendString() {
    setup_editor();
    
    editorInsertRow(0, "Hello", 5);
    erow *row = &E.row[0];
    
    // 文字列を追加
    editorRowAppendString(row, " World", 6);
    TEST_ASSERT_EQ_INT(11, row->size);
    TEST_ASSERT_STR_EQ("Hello World", row->chars);
    
    // 空文字列を追加（変化なし）
    editorRowAppendString(row, "", 0);
    TEST_ASSERT_EQ_INT(11, row->size);
    
    // 日本語を追加
    editorRowAppendString(row, "！", 3);
    TEST_ASSERT_EQ_INT(14, row->size);
    
    cleanup_editor();
}

/* タブとマルチバイト文字の混在テスト */
void test_mixed_tab_multibyte() {
    setup_editor();
    
    editorInsertRow(0, "日本語\tタブ", 15);  // 日本語(9) + タブ(1) + タブ(6) = 16?
    erow *row = &E.row[0];
    
    // タブ位置の計算が正しいか確認
    int rx_after_japanese = editorRowCxToRx(row, 9);  // "日本語"の後
    TEST_ASSERT_EQ_INT(6, rx_after_japanese);  // 日本語は表示幅6
    
    int rx_after_tab = editorRowCxToRx(row, 10);  // タブの後
    TEST_ASSERT_EQ_INT(8, rx_after_tab);  // 次のタブストップ位置
    
    cleanup_editor();
}

int main() {
    TEST_GROUP("Row Operations");
    
    RUN_TEST(test_editorRowCxToRx_ascii);
    RUN_TEST(test_editorRowCxToRx_with_tabs);
    RUN_TEST(test_editorRowCxToRx_multibyte);
    RUN_TEST(test_editorRowRxToCx_ascii);
    RUN_TEST(test_editorUpdateRow_tabs);
    RUN_TEST(test_editorInsertRow);
    RUN_TEST(test_editorDelRow);
    RUN_TEST(test_editorRowInsertChar);
    RUN_TEST(test_editorRowDelChar);
    RUN_TEST(test_editorRowAppendString);
    RUN_TEST(test_mixed_tab_multibyte);
    
    TEST_SUMMARY();
}