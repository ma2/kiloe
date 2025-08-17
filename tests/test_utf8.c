/**
 * test_utf8.c - UTF-8処理関数のテスト
 */

#include "minunit.h"
#include "../src/kiloe.h"

/* UTF-8継続バイトのテスト */
void test_is_utf8_continuation() {
    // 継続バイト (10xxxxxx)
    TEST_ASSERT_TRUE(is_utf8_continuation(0x80));  // 10000000
    TEST_ASSERT_TRUE(is_utf8_continuation(0xBF));  // 10111111
    TEST_ASSERT_TRUE(is_utf8_continuation(0x90));  // 10010000
    
    // 非継続バイト
    TEST_ASSERT_FALSE(is_utf8_continuation(0x00)); // 00000000 (ASCII)
    TEST_ASSERT_FALSE(is_utf8_continuation(0x7F)); // 01111111 (ASCII)
    TEST_ASSERT_FALSE(is_utf8_continuation(0xC0)); // 11000000 (2バイト開始)
    TEST_ASSERT_FALSE(is_utf8_continuation(0xE0)); // 11100000 (3バイト開始)
    TEST_ASSERT_FALSE(is_utf8_continuation(0xF0)); // 11110000 (4バイト開始)
}

/* UTF-8文字長の取得テスト */
void test_utf8_char_len() {
    // 1バイト文字 (ASCII)
    TEST_ASSERT_EQ_INT(1, utf8_char_len('A'));
    TEST_ASSERT_EQ_INT(1, utf8_char_len('0'));
    TEST_ASSERT_EQ_INT(1, utf8_char_len(' '));
    
    // 2バイト文字 (ラテン拡張など)
    TEST_ASSERT_EQ_INT(2, utf8_char_len(0xC2)); // 11000010
    TEST_ASSERT_EQ_INT(2, utf8_char_len(0xDF)); // 11011111
    
    // 3バイト文字 (日本語など)
    TEST_ASSERT_EQ_INT(3, utf8_char_len(0xE0)); // 11100000
    TEST_ASSERT_EQ_INT(3, utf8_char_len(0xEF)); // 11101111
    
    // 4バイト文字 (絵文字など)
    TEST_ASSERT_EQ_INT(4, utf8_char_len(0xF0)); // 11110000
    TEST_ASSERT_EQ_INT(4, utf8_char_len(0xF4)); // 11110100
}

/* 文字幅取得のテスト */
void test_get_char_width() {
    // ASCII文字は幅1
    TEST_ASSERT_EQ_INT(1, get_char_width("Hello", 0));
    TEST_ASSERT_EQ_INT(1, get_char_width("123", 0));
    
    // 日本語文字は幅2（3バイトUTF-8）
    char japanese[] = "あいう";  // あ = E3 81 82
    TEST_ASSERT_EQ_INT(2, get_char_width(japanese, 0));
    
    // タブは幅1として扱われる（特殊文字）
    TEST_ASSERT_EQ_INT(1, get_char_width("\t", 0));
}

/* 次の文字位置への移動テスト */
void test_move_to_next_char() {
    char ascii[] = "Hello";
    char japanese[] = "あいう";  // 各文字3バイト
    char mixed[] = "Aあ1い";
    
    // ASCII文字列
    TEST_ASSERT_EQ_INT(1, move_to_next_char(ascii, 0, 5));
    TEST_ASSERT_EQ_INT(2, move_to_next_char(ascii, 1, 5));
    TEST_ASSERT_EQ_INT(5, move_to_next_char(ascii, 4, 5));
    TEST_ASSERT_EQ_INT(5, move_to_next_char(ascii, 5, 5)); // 範囲外
    
    // 日本語文字列
    TEST_ASSERT_EQ_INT(3, move_to_next_char(japanese, 0, 9));  // "あ"の次
    TEST_ASSERT_EQ_INT(6, move_to_next_char(japanese, 3, 9));  // "い"の次
    TEST_ASSERT_EQ_INT(9, move_to_next_char(japanese, 6, 9));  // "う"の次
    
    // 混在文字列
    TEST_ASSERT_EQ_INT(1, move_to_next_char(mixed, 0, 8));  // 'A'の次
    TEST_ASSERT_EQ_INT(4, move_to_next_char(mixed, 1, 8));  // 'あ'の次
    TEST_ASSERT_EQ_INT(5, move_to_next_char(mixed, 4, 8));  // '1'の次
    TEST_ASSERT_EQ_INT(8, move_to_next_char(mixed, 5, 8));  // 'い'の次
}

/* 前の文字位置への移動テスト */
void test_move_to_prev_char() {
    char ascii[] = "Hello";
    char japanese[] = "あいう";
    char mixed[] = "Aあ1い";
    
    // ASCII文字列
    TEST_ASSERT_EQ_INT(0, move_to_prev_char(ascii, 1));
    TEST_ASSERT_EQ_INT(1, move_to_prev_char(ascii, 2));
    TEST_ASSERT_EQ_INT(0, move_to_prev_char(ascii, 0)); // 先頭
    
    // 日本語文字列
    TEST_ASSERT_EQ_INT(0, move_to_prev_char(japanese, 3));  // "い"から"あ"
    TEST_ASSERT_EQ_INT(3, move_to_prev_char(japanese, 6));  // "う"から"い"
    TEST_ASSERT_EQ_INT(0, move_to_prev_char(japanese, 1));  // 継続バイトから先頭
    TEST_ASSERT_EQ_INT(0, move_to_prev_char(japanese, 2));  // 継続バイトから先頭
    
    // 混在文字列
    TEST_ASSERT_EQ_INT(0, move_to_prev_char(mixed, 1));   // 'あ'から'A'
    TEST_ASSERT_EQ_INT(1, move_to_prev_char(mixed, 4));   // '1'から'あ'
    TEST_ASSERT_EQ_INT(4, move_to_prev_char(mixed, 5));   // 'い'から'1'
}

int main() {
    TEST_GROUP("UTF-8 Functions");
    
    RUN_TEST(test_is_utf8_continuation);
    RUN_TEST(test_utf8_char_len);
    RUN_TEST(test_get_char_width);
    RUN_TEST(test_move_to_next_char);
    RUN_TEST(test_move_to_prev_char);
    
    TEST_SUMMARY();
}