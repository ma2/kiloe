/**
 * test_syntax.c - シンタックスハイライト関数のテスト
 */

#define _GNU_SOURCE  /* strdup用 */
#include "minunit.h"
#include "../src/kiloe.h"
#include <stdlib.h>
#include <string.h>

/* 外部変数の宣言 */
extern struct editorConfig E;
extern struct editorSyntax HLDB[];

/* HLDBエントリ数の取得テスト */
void test_getHLDBEntries() {
    int entries = getHLDBEntries();
    
    // 現在は6つの言語サポート + NULL終端
    TEST_ASSERT_EQ_INT(6, entries);
    
    // 各エントリの存在確認
    TEST_ASSERT_NOT_NULL(HLDB[0].filetype);
    TEST_ASSERT_STR_EQ("c", HLDB[0].filetype);
    TEST_ASSERT_STR_EQ("python", HLDB[1].filetype);
    TEST_ASSERT_STR_EQ("javascript", HLDB[2].filetype);
    TEST_ASSERT_STR_EQ("ruby", HLDB[3].filetype);
    TEST_ASSERT_STR_EQ("html", HLDB[4].filetype);
    TEST_ASSERT_STR_EQ("markdown", HLDB[5].filetype);
}

/* ファイル拡張子マッチングのテスト */
void test_file_extension_matching() {
    // テスト用にE.filenameを設定
    
    // C言語ファイル
    E.filename = strdup("test.c");
    E.syntax = NULL;
    editorSelectSyntaxHighlight();
    TEST_ASSERT_NOT_NULL(E.syntax);
    TEST_ASSERT_STR_EQ("c", E.syntax->filetype);
    free(E.filename);
    
    // Pythonファイル
    E.filename = strdup("script.py");
    E.syntax = NULL;
    editorSelectSyntaxHighlight();
    TEST_ASSERT_NOT_NULL(E.syntax);
    TEST_ASSERT_STR_EQ("python", E.syntax->filetype);
    free(E.filename);
    
    // JavaScriptファイル
    E.filename = strdup("app.js");
    E.syntax = NULL;
    editorSelectSyntaxHighlight();
    TEST_ASSERT_NOT_NULL(E.syntax);
    TEST_ASSERT_STR_EQ("javascript", E.syntax->filetype);
    free(E.filename);
    
    // Rubyファイル
    E.filename = strdup("script.rb");
    E.syntax = NULL;
    editorSelectSyntaxHighlight();
    TEST_ASSERT_NOT_NULL(E.syntax);
    TEST_ASSERT_STR_EQ("ruby", E.syntax->filetype);
    free(E.filename);
    
    // HTMLファイル
    E.filename = strdup("index.html");
    E.syntax = NULL;
    editorSelectSyntaxHighlight();
    TEST_ASSERT_NOT_NULL(E.syntax);
    TEST_ASSERT_STR_EQ("html", E.syntax->filetype);
    free(E.filename);
    
    // Markdownファイル
    E.filename = strdup("README.md");
    E.syntax = NULL;
    editorSelectSyntaxHighlight();
    TEST_ASSERT_NOT_NULL(E.syntax);
    TEST_ASSERT_STR_EQ("markdown", E.syntax->filetype);
    free(E.filename);
    
    // 不明な拡張子
    E.filename = strdup("file.xyz");
    E.syntax = NULL;
    editorSelectSyntaxHighlight();
    TEST_ASSERT("Unknown extension should not match", E.syntax == NULL);
    free(E.filename);
}

/* コメント設定のテスト */
void test_comment_settings() {
    
    // C言語のコメント設定
    TEST_ASSERT_STR_EQ("//", HLDB[0].singleline_comment_start);
    TEST_ASSERT_STR_EQ("/*", HLDB[0].multiline_comment_start);
    TEST_ASSERT_STR_EQ("*/", HLDB[0].multiline_comment_end);
    
    // Pythonのコメント設定
    TEST_ASSERT_STR_EQ("#", HLDB[1].singleline_comment_start);
    TEST_ASSERT("Python has no multiline comment", 
               HLDB[1].multiline_comment_start == NULL);
    
    // HTMLのコメント設定
    TEST_ASSERT("HTML has no single line comment", 
               HLDB[4].singleline_comment_start == NULL);
    TEST_ASSERT_STR_EQ("<!--", HLDB[4].multiline_comment_start);
    TEST_ASSERT_STR_EQ("-->", HLDB[4].multiline_comment_end);
}

/* ハイライトフラグのテスト */
void test_highlight_flags() {
    // C言語: 数値と文字列をハイライト
    TEST_ASSERT_TRUE(HLDB[0].flags & HL_HIGHLIGHT_NUMBERS);
    TEST_ASSERT_TRUE(HLDB[0].flags & HL_HIGHLIGHT_STRINGS);
    
    // HTML: 文字列のみハイライト
    TEST_ASSERT_FALSE(HLDB[4].flags & HL_HIGHLIGHT_NUMBERS);
    TEST_ASSERT_TRUE(HLDB[4].flags & HL_HIGHLIGHT_STRINGS);
    
    // Markdown: ハイライトなし
    TEST_ASSERT_EQ_INT(0, HLDB[5].flags);
}

int main() {
    TEST_GROUP("Syntax Highlighting");
    
    // E構造体の初期化
    memset(&E, 0, sizeof(E));
    E.numrows = 0;
    E.row = NULL;
    
    RUN_TEST(test_getHLDBEntries);
    RUN_TEST(test_file_extension_matching);
    RUN_TEST(test_comment_settings);
    RUN_TEST(test_highlight_flags);
    
    TEST_SUMMARY();
}