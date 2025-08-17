/**
 * test_buffer.c - バッファ操作関数のテスト
 */

#define _GNU_SOURCE
#include "minunit.h"
#include "../src/kiloe.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* abuf初期化のテスト */
void test_abuf_init() {
    struct abuf ab = ABUF_INIT;
    
    TEST_ASSERT_EQ_INT(0, ab.len);
    TEST_ASSERT("Initial buffer should be NULL", ab.b == NULL);
}

/* バッファへの文字列追加テスト */
void test_abAppend_single() {
    struct abuf ab = ABUF_INIT;
    const char* str = "Hello";
    
    abAppend(&ab, str, strlen(str));
    
    TEST_ASSERT_EQ_INT(5, ab.len);
    TEST_ASSERT_NOT_NULL(ab.b);
    TEST_ASSERT("Buffer content should match", memcmp(ab.b, str, 5) == 0);
    
    abFree(&ab);
}

/* バッファへの複数追加テスト */
void test_abAppend_multiple() {
    struct abuf ab = ABUF_INIT;
    
    abAppend(&ab, "Hello", 5);
    abAppend(&ab, " ", 1);
    abAppend(&ab, "World", 5);
    
    TEST_ASSERT_EQ_INT(11, ab.len);
    TEST_ASSERT("Buffer should contain concatenated string", 
               memcmp(ab.b, "Hello World", 11) == 0);
    
    abFree(&ab);
}

/* 空文字列の追加テスト */
void test_abAppend_empty() {
    struct abuf ab = ABUF_INIT;
    
    abAppend(&ab, "Test", 4);
    int original_len = ab.len;
    
    // 空文字列を追加
    abAppend(&ab, "", 0);
    
    TEST_ASSERT_EQ_INT(original_len, ab.len);
    TEST_ASSERT("Buffer should remain unchanged", 
               memcmp(ab.b, "Test", 4) == 0);
    
    abFree(&ab);
}

/* 大きなバッファの追加テスト */
void test_abAppend_large() {
    struct abuf ab = ABUF_INIT;
    
    // 1000バイトのデータを作成
    char large_data[1000];
    for (int i = 0; i < 1000; i++) {
        large_data[i] = 'A' + (i % 26);
    }
    
    abAppend(&ab, large_data, 1000);
    
    TEST_ASSERT_EQ_INT(1000, ab.len);
    TEST_ASSERT("Large buffer should match", 
               memcmp(ab.b, large_data, 1000) == 0);
    
    abFree(&ab);
}

/* 日本語文字列の追加テスト */
void test_abAppend_multibyte() {
    struct abuf ab = ABUF_INIT;
    const char* japanese = "こんにちは";  // 15バイト（各文字3バイト）
    
    abAppend(&ab, japanese, strlen(japanese));
    
    TEST_ASSERT_EQ_INT(15, ab.len);
    TEST_ASSERT("Multibyte string should be preserved", 
               memcmp(ab.b, japanese, 15) == 0);
    
    abFree(&ab);
}

/* バッファ解放のテスト */
void test_abFree() {
    struct abuf ab = ABUF_INIT;
    
    abAppend(&ab, "Test", 4);
    TEST_ASSERT_NOT_NULL(ab.b);
    
    abFree(&ab);
    
    TEST_ASSERT_EQ_INT(0, ab.len);
    TEST_ASSERT("Buffer should be NULL after free", ab.b == NULL);
}

/* NULLポインタ処理のテスト */
void test_abAppend_null_handling() {
    struct abuf ab = ABUF_INIT;
    
    // 最初に有効なデータを追加
    abAppend(&ab, "Valid", 5);
    int original_len = ab.len;
    
    // NULLポインタを渡した場合（実装によってはクラッシュする可能性）
    // この動作は実装依存なので、実装を確認してからテストを調整
    // abAppend(&ab, NULL, 0);  // これは危険
    
    // 長さ0の追加は安全
    abAppend(&ab, "Test", 0);
    TEST_ASSERT_EQ_INT(original_len, ab.len);
    
    abFree(&ab);
}

/* エスケープシーケンスの追加テスト */
void test_abAppend_escape_sequences() {
    struct abuf ab = ABUF_INIT;
    const char* escape = "\x1b[2J\x1b[H";  // 画面クリアとカーソルホーム
    
    abAppend(&ab, escape, strlen(escape));
    
    TEST_ASSERT_EQ_INT(7, ab.len);  // \x1b[2J = 4バイト, \x1b[H = 3バイト
    TEST_ASSERT("Escape sequences should be preserved", 
               memcmp(ab.b, escape, 7) == 0);
    
    abFree(&ab);
}

/* 連続した再割り当てのストレステスト */
void test_abAppend_stress() {
    struct abuf ab = ABUF_INIT;
    
    // 100回の追加を行う
    for (int i = 0; i < 100; i++) {
        char buf[10];
        snprintf(buf, sizeof(buf), "%d,", i);
        abAppend(&ab, buf, strlen(buf));
    }
    
    TEST_ASSERT_TRUE(ab.len > 0);
    TEST_ASSERT_NOT_NULL(ab.b);
    
    // 最初の数文字を確認
    TEST_ASSERT("Should start with 0,", memcmp(ab.b, "0,", 2) == 0);
    
    abFree(&ab);
}

int main() {
    TEST_GROUP("Buffer Management (abuf)");
    
    RUN_TEST(test_abuf_init);
    RUN_TEST(test_abAppend_single);
    RUN_TEST(test_abAppend_multiple);
    RUN_TEST(test_abAppend_empty);
    RUN_TEST(test_abAppend_large);
    RUN_TEST(test_abAppend_multibyte);
    RUN_TEST(test_abFree);
    RUN_TEST(test_abAppend_null_handling);
    RUN_TEST(test_abAppend_escape_sequences);
    RUN_TEST(test_abAppend_stress);
    
    TEST_SUMMARY();
}