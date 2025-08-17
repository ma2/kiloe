/**
 * minunit.h - 最小限の単体テストフレームワーク
 * 
 * 軽量で依存関係のないテストフレームワーク
 * assertを使わず、テストの継続実行が可能
 */

#ifndef MINUNIT_H
#define MINUNIT_H

#include <stdio.h>
#include <string.h>

/* テスト統計情報 */
static int tests_run = 0;
static int tests_failed = 0;
static int tests_passed = 0;

/* 基本的なアサーションマクロ */
#define TEST_ASSERT(message, test) do { \
    tests_run++; \
    if (!(test)) { \
        printf("  ✗ FAIL: %s:%d: %s\n", __FILE__, __LINE__, message); \
        tests_failed++; \
    } else { \
        tests_passed++; \
    } \
} while (0)

/* 整数比較 */
#define TEST_ASSERT_EQ_INT(expected, actual) do { \
    tests_run++; \
    if ((expected) != (actual)) { \
        printf("  ✗ FAIL: %s:%d: expected %d, got %d\n", \
               __FILE__, __LINE__, (int)(expected), (int)(actual)); \
        tests_failed++; \
    } else { \
        tests_passed++; \
    } \
} while (0)

/* 文字列比較 */
#define TEST_ASSERT_STR_EQ(expected, actual) do { \
    tests_run++; \
    if (strcmp((expected), (actual)) != 0) { \
        printf("  ✗ FAIL: %s:%d: expected '%s', got '%s'\n", \
               __FILE__, __LINE__, (expected), (actual)); \
        tests_failed++; \
    } else { \
        tests_passed++; \
    } \
} while (0)

/* ポインタNULLチェック */
#define TEST_ASSERT_NOT_NULL(ptr) do { \
    tests_run++; \
    if ((ptr) == NULL) { \
        printf("  ✗ FAIL: %s:%d: pointer is NULL\n", __FILE__, __LINE__); \
        tests_failed++; \
    } else { \
        tests_passed++; \
    } \
} while (0)

/* 真偽値チェック */
#define TEST_ASSERT_TRUE(condition) TEST_ASSERT(#condition " should be true", condition)
#define TEST_ASSERT_FALSE(condition) TEST_ASSERT(#condition " should be false", !(condition))

/* テスト実行マクロ */
#define RUN_TEST(test_func) do { \
    printf("\n▶ Running %s...\n", #test_func); \
    test_func(); \
} while (0)

/* テストグループ */
#define TEST_GROUP(name) printf("\n━━━ Test Group: %s ━━━\n", name)

/* テストサマリー表示 */
#define TEST_SUMMARY() do { \
    printf("\n"); \
    printf("╔════════════════════════════╗\n"); \
    printf("║     TEST SUMMARY           ║\n"); \
    printf("╠════════════════════════════╣\n"); \
    printf("║ Total:   %4d tests        ║\n", tests_run); \
    printf("║ Passed:  %4d tests ✓      ║\n", tests_passed); \
    printf("║ Failed:  %4d tests ✗      ║\n", tests_failed); \
    printf("║ Success: %6.1f%%          ║\n", \
           tests_run > 0 ? (100.0 * tests_passed / tests_run) : 0); \
    printf("╚════════════════════════════╝\n"); \
    return tests_failed; \
} while (0)

#endif /* MINUNIT_H */