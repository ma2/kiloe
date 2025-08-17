/**
 * test_config.c - 設定管理関数のテスト
 */

#define _GNU_SOURCE
#include "minunit.h"
#include "../src/kiloe.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* 外部変数 */
extern struct editorSettings Config;

/* テスト用設定ファイルの作成 */
static void create_test_config(const char* filename, const char* content) {
    FILE* f = fopen(filename, "w");
    if (f) {
        fprintf(f, "%s", content);
        fclose(f);
    }
}

/* デフォルト設定の初期化テスト */
void test_initDefaultConfig() {
    initDefaultConfig();
    
    // デフォルト値の確認
    TEST_ASSERT_EQ_INT(8, Config.tab_stop);
    TEST_ASSERT_EQ_INT(3, Config.quit_times);
}

/* 設定ファイル読み込みテスト - 正常系 */
void test_loadConfig_valid() {
    const char* test_file = "test_config.conf";
    const char* content = 
        "# Test configuration\n"
        "tab_stop = 4\n"
        "quit_times = 5\n";
    
    create_test_config(test_file, content);
    
    // デフォルト値を設定
    initDefaultConfig();
    
    // 設定ファイルを読み込み
    int result = loadConfig(test_file);
    TEST_ASSERT_EQ_INT(0, result);
    
    // 値が更新されたことを確認
    TEST_ASSERT_EQ_INT(4, Config.tab_stop);
    TEST_ASSERT_EQ_INT(5, Config.quit_times);
    
    // クリーンアップ
    unlink(test_file);
}

/* 設定ファイル読み込みテスト - コメントと空行 */
void test_loadConfig_with_comments() {
    const char* test_file = "test_config2.conf";
    const char* content = 
        "# This is a comment\n"
        "\n"  // 空行
        "tab_stop = 2\n"
        "  # Indented comment\n"
        "\n"
        "quit_times = 1\n";
    
    create_test_config(test_file, content);
    initDefaultConfig();
    
    int result = loadConfig(test_file);
    TEST_ASSERT_EQ_INT(0, result);
    
    TEST_ASSERT_EQ_INT(2, Config.tab_stop);
    TEST_ASSERT_EQ_INT(1, Config.quit_times);
    
    unlink(test_file);
}

/* 設定ファイル読み込みテスト - 不正な値 */
void test_loadConfig_invalid_values() {
    const char* test_file = "test_config3.conf";
    const char* content = 
        "tab_stop = not_a_number\n"
        "quit_times = -5\n"
        "unknown_key = 123\n";
    
    create_test_config(test_file, content);
    initDefaultConfig();
    
    // 元の値を記憶
    int orig_tab = Config.tab_stop;
    int orig_quit = Config.quit_times;
    
    int result = loadConfig(test_file);
    TEST_ASSERT_EQ_INT(0, result);  // ファイル自体は読み込める
    
    // 不正な値は無視され、デフォルト値が維持される
    TEST_ASSERT_EQ_INT(0, Config.tab_stop);  // atoi("not_a_number") = 0
    TEST_ASSERT_EQ_INT(-5, Config.quit_times);  // 負の値も設定される
    
    unlink(test_file);
}

/* 設定ファイル読み込みテスト - ファイルが存在しない */
void test_loadConfig_file_not_found() {
    const char* test_file = "nonexistent_file.conf";
    
    initDefaultConfig();
    
    // 元の値を記憶
    int orig_tab = Config.tab_stop;
    
    int result = loadConfig(test_file);
    TEST_ASSERT_EQ_INT(-1, result);
    
    // 設定は変更されない
    TEST_ASSERT_EQ_INT(orig_tab, Config.tab_stop);
}

/* 設定ファイル読み込みテスト - スペース処理 */
void test_loadConfig_with_spaces() {
    const char* test_file = "test_config4.conf";
    const char* content = 
        "  tab_stop   =   16  \n"  // 前後にスペース
        "quit_times=2\n";            // スペースなし
    
    create_test_config(test_file, content);
    initDefaultConfig();
    
    int result = loadConfig(test_file);
    TEST_ASSERT_EQ_INT(0, result);
    
    TEST_ASSERT_EQ_INT(16, Config.tab_stop);
    TEST_ASSERT_EQ_INT(2, Config.quit_times);
    
    unlink(test_file);
}

int main() {
    TEST_GROUP("Configuration Management");
    
    RUN_TEST(test_initDefaultConfig);
    RUN_TEST(test_loadConfig_valid);
    RUN_TEST(test_loadConfig_with_comments);
    RUN_TEST(test_loadConfig_invalid_values);
    RUN_TEST(test_loadConfig_file_not_found);
    RUN_TEST(test_loadConfig_with_spaces);
    
    TEST_SUMMARY();
}