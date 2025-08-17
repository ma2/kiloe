/**
 * test_line_numbers.c - 行番号表示機能のテスト
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
    E.screenrows = 24;
    E.screencols = 80;
    
    // デフォルト設定
    Config.tab_stop = 8;
    Config.show_line_numbers = 0;
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

/* 行番号表示無効時のテスト */
void test_line_numbers_disabled() {
    setup_editor();
    
    Config.show_line_numbers = 0;
    
    editorInsertRow(0, "Line 1", 6);
    editorInsertRow(1, "Line 2", 6);
    
    // 行番号表示が無効の場合、通常通り動作することを確認
    TEST_ASSERT_EQ_INT(2, E.numrows);
    TEST_ASSERT_STR_EQ("Line 1", E.row[0].chars);
    TEST_ASSERT_STR_EQ("Line 2", E.row[1].chars);
    
    cleanup_editor();
}

/* 行番号表示有効時のテスト */
void test_line_numbers_enabled() {
    setup_editor();
    
    Config.show_line_numbers = 1;
    
    // 10行追加（行番号は1桁 + スペース = 2文字）
    for (int i = 0; i < 10; i++) {
        char line[20];
        snprintf(line, sizeof(line), "Line %d", i + 1);
        editorInsertRow(i, line, strlen(line));
    }
    
    TEST_ASSERT_EQ_INT(10, E.numrows);
    
    cleanup_editor();
}

/* 行番号表示有効時（100行）のテスト */
void test_line_numbers_100_lines() {
    setup_editor();
    
    Config.show_line_numbers = 1;
    
    // 100行追加（行番号は3桁 + スペース = 4文字）
    for (int i = 0; i < 100; i++) {
        char line[20];
        snprintf(line, sizeof(line), "Line %d", i + 1);
        editorInsertRow(i, line, strlen(line));
    }
    
    TEST_ASSERT_EQ_INT(100, E.numrows);
    
    cleanup_editor();
}

/* 設定ファイルからの行番号設定読み込みテスト */
void test_config_line_numbers() {
    setup_editor();
    
    // デフォルト値確認
    initDefaultConfig();
    TEST_ASSERT_EQ_INT(0, Config.show_line_numbers);
    
    // 設定ファイル作成
    const char* test_file = "test_line_config.conf";
    FILE* f = fopen(test_file, "w");
    if (f) {
        fprintf(f, "show_line_numbers = 1\n");
        fclose(f);
    }
    
    // 設定読み込み
    int result = loadConfig(test_file);
    TEST_ASSERT_EQ_INT(0, result);
    TEST_ASSERT_EQ_INT(1, Config.show_line_numbers);
    
    // クリーンアップ
    unlink(test_file);
    cleanup_editor();
}

int main() {
    TEST_GROUP("Line Numbers Display");
    
    RUN_TEST(test_line_numbers_disabled);
    RUN_TEST(test_line_numbers_enabled);
    RUN_TEST(test_line_numbers_100_lines);
    RUN_TEST(test_config_line_numbers);
    
    TEST_SUMMARY();
}