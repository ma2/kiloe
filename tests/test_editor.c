/**
 * test_editor.c - エディタ操作関数のテスト
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
    E.cx = 0;
    E.cy = 0;
    E.rx = 0;
    
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

/* editorInsertCharのテスト - 空のエディタ */
void test_editorInsertChar_empty() {
    setup_editor();
    
    TEST_ASSERT_EQ_INT(0, E.numrows);
    
    // 文字を挿入（自動的に行が作成される）
    editorInsertChar('A');
    
    TEST_ASSERT_EQ_INT(1, E.numrows);
    TEST_ASSERT_STR_EQ("A", E.row[0].chars);
    TEST_ASSERT_EQ_INT(1, E.cx);  // カーソルが右に移動
    TEST_ASSERT_EQ_INT(2, E.dirty);  // ダーティフラグ（行作成1 + 文字挿入1）
    
    cleanup_editor();
}

/* editorInsertCharのテスト - 既存行への挿入 */
void test_editorInsertChar_existing_line() {
    setup_editor();
    
    // 初期行を作成
    editorInsertRow(0, "Hello", 5);
    E.cy = 0;
    E.cx = 5;  // 行末に移動
    
    editorInsertChar('!');
    
    TEST_ASSERT_STR_EQ("Hello!", E.row[0].chars);
    TEST_ASSERT_EQ_INT(6, E.cx);
    
    cleanup_editor();
}

/* editorInsertCharのテスト - 行の途中への挿入 */
void test_editorInsertChar_middle() {
    setup_editor();
    
    editorInsertRow(0, "Hllo", 4);
    E.cy = 0;
    E.cx = 1;  // 'H'と'l'の間
    
    editorInsertChar('e');
    
    TEST_ASSERT_STR_EQ("Hello", E.row[0].chars);
    TEST_ASSERT_EQ_INT(2, E.cx);  // カーソルが挿入文字の後に移動
    
    cleanup_editor();
}

/* editorInsertNewLineのテスト - 行頭 */
void test_editorInsertNewLine_beginning() {
    setup_editor();
    
    editorInsertRow(0, "Hello", 5);
    E.cy = 0;
    E.cx = 0;  // 行頭
    
    editorInsertNewLine();
    
    TEST_ASSERT_EQ_INT(2, E.numrows);
    TEST_ASSERT_STR_EQ("", E.row[0].chars);     // 新しい空行
    TEST_ASSERT_STR_EQ("Hello", E.row[1].chars); // 元の行
    TEST_ASSERT_EQ_INT(1, E.cy);  // 次の行に移動
    TEST_ASSERT_EQ_INT(0, E.cx);  // 行頭に移動
    
    cleanup_editor();
}

/* editorInsertNewLineのテスト - 行の途中 */
void test_editorInsertNewLine_middle() {
    setup_editor();
    
    editorInsertRow(0, "Hello World", 11);
    E.cy = 0;
    E.cx = 5;  // "Hello"と" World"の間
    
    editorInsertNewLine();
    
    TEST_ASSERT_EQ_INT(2, E.numrows);
    TEST_ASSERT_STR_EQ("Hello", E.row[0].chars);   // 前半
    TEST_ASSERT_STR_EQ(" World", E.row[1].chars);  // 後半
    TEST_ASSERT_EQ_INT(1, E.cy);
    TEST_ASSERT_EQ_INT(0, E.cx);
    
    cleanup_editor();
}

/* editorDelCharのテスト - 文字削除 */
void test_editorDelChar_character() {
    setup_editor();
    
    editorInsertRow(0, "Hello!", 6);
    E.cy = 0;
    E.cx = 6;  // 末尾
    
    editorDelChar();  // '!'を削除
    
    TEST_ASSERT_STR_EQ("Hello", E.row[0].chars);
    TEST_ASSERT_EQ_INT(5, E.cx);  // カーソルが左に移動
    
    cleanup_editor();
}

/* editorDelCharのテスト - 行の結合 */
void test_editorDelChar_line_join() {
    setup_editor();
    
    editorInsertRow(0, "Hello", 5);
    editorInsertRow(1, "World", 5);
    E.cy = 1;
    E.cx = 0;  // 2行目の先頭
    
    editorDelChar();  // 改行を削除して行を結合
    
    TEST_ASSERT_EQ_INT(1, E.numrows);
    TEST_ASSERT_STR_EQ("HelloWorld", E.row[0].chars);
    TEST_ASSERT_EQ_INT(0, E.cy);  // 前の行に移動
    TEST_ASSERT_EQ_INT(5, E.cx);  // 結合位置に移動
    
    cleanup_editor();
}

/* editorDelCharのテスト - 日本語文字削除 */
void test_editorDelChar_multibyte() {
    setup_editor();
    
    editorInsertRow(0, "あいう", 9);  // 各文字3バイト
    E.cy = 0;
    E.cx = 9;  // 末尾
    
    editorDelChar();  // "う"を削除
    
    TEST_ASSERT_STR_EQ("あい", E.row[0].chars);
    TEST_ASSERT_EQ_INT(6, E.cx);  // "う"の開始位置に移動
    
    cleanup_editor();
}

/* editorDelCharのテスト - 境界条件 */
void test_editorDelChar_boundaries() {
    setup_editor();
    
    // ファイル先頭での削除（何も起こらない）
    editorInsertRow(0, "Hello", 5);
    E.cy = 0;
    E.cx = 0;
    
    editorDelChar();
    
    TEST_ASSERT_STR_EQ("Hello", E.row[0].chars);
    TEST_ASSERT_EQ_INT(0, E.cx);
    TEST_ASSERT_EQ_INT(0, E.cy);
    
    // ファイル末尾での削除
    E.cy = E.numrows;  // 最終行の次
    E.cx = 0;
    
    editorDelChar();
    
    TEST_ASSERT_EQ_INT(1, E.numrows);  // 変化なし
    
    cleanup_editor();
}

/* 複合操作のテスト - 挿入と削除の組み合わせ */
void test_combined_operations() {
    setup_editor();
    
    // 文字の挿入
    editorInsertChar('H');
    editorInsertChar('e');
    editorInsertChar('l');
    editorInsertChar('l');
    editorInsertChar('o');
    
    TEST_ASSERT_STR_EQ("Hello", E.row[0].chars);
    TEST_ASSERT_EQ_INT(5, E.cx);
    
    // 改行の挿入
    editorInsertNewLine();
    
    TEST_ASSERT_EQ_INT(2, E.numrows);
    TEST_ASSERT_EQ_INT(1, E.cy);
    TEST_ASSERT_EQ_INT(0, E.cx);
    
    // 2行目に文字挿入
    editorInsertChar('W');
    editorInsertChar('o');
    editorInsertChar('r');
    editorInsertChar('l');
    editorInsertChar('d');
    
    TEST_ASSERT_STR_EQ("World", E.row[1].chars);
    
    // 文字削除
    editorDelChar();  // 'd'を削除
    TEST_ASSERT_STR_EQ("Worl", E.row[1].chars);
    
    // 行の結合（改行削除）
    E.cx = 0;  // 行頭に移動
    editorDelChar();
    
    TEST_ASSERT_EQ_INT(1, E.numrows);
    TEST_ASSERT_STR_EQ("HelloWorl", E.row[0].chars);
    
    cleanup_editor();
}

/* ダーティフラグのテスト */
void test_dirty_flag() {
    setup_editor();
    
    TEST_ASSERT_EQ_INT(0, E.dirty);
    
    // 文字挿入でダーティフラグが設定される
    editorInsertChar('A');
    TEST_ASSERT_TRUE(E.dirty > 0);
    
    int dirty_after_insert = E.dirty;
    
    // 改行挿入でダーティフラグが増加
    editorInsertNewLine();
    TEST_ASSERT_TRUE(E.dirty > dirty_after_insert);
    
    int dirty_after_newline = E.dirty;
    
    // 文字削除でダーティフラグが増加
    editorInsertChar('B');  // 文字を追加してから削除
    editorDelChar();
    TEST_ASSERT_TRUE(E.dirty > dirty_after_newline);
    
    cleanup_editor();
}

/* UTF-8とタブの混在テスト */
void test_utf8_tab_operations() {
    setup_editor();
    
    // 注意: editorInsertCharは1バイトずつしか処理しないため、
    // 日本語文字は正しく挿入されない。ここではASCII文字でテスト
    editorInsertChar('A');  // ASCII文字
    editorInsertChar('\t');  // タブ
    editorInsertChar('B');
    
    TEST_ASSERT_EQ_INT(1, E.numrows);
    TEST_ASSERT_EQ_INT(3, E.cx);  // A(1) + タブ(1) + B(1)
    
    // 文字削除
    editorDelChar();  // 'B'を削除
    TEST_ASSERT_EQ_INT(2, E.cx);  // A(1) + タブ(1)
    
    editorDelChar();  // タブを削除
    TEST_ASSERT_EQ_INT(1, E.cx);  // A(1)
    
    editorDelChar();  // 'A'を削除
    TEST_ASSERT_EQ_INT(0, E.cx);
    TEST_ASSERT_STR_EQ("", E.row[0].chars);
    
    cleanup_editor();
}

int main() {
    TEST_GROUP("Editor Operations");
    
    RUN_TEST(test_editorInsertChar_empty);
    RUN_TEST(test_editorInsertChar_existing_line);
    RUN_TEST(test_editorInsertChar_middle);
    RUN_TEST(test_editorInsertNewLine_beginning);
    RUN_TEST(test_editorInsertNewLine_middle);
    RUN_TEST(test_editorDelChar_character);
    RUN_TEST(test_editorDelChar_line_join);
    RUN_TEST(test_editorDelChar_multibyte);
    RUN_TEST(test_editorDelChar_boundaries);
    RUN_TEST(test_combined_operations);
    RUN_TEST(test_dirty_flag);
    RUN_TEST(test_utf8_tab_operations);
    
    TEST_SUMMARY();
}