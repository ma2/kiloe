/**
 * buffer.c - 追加バッファ機能
 * 
 * 効率的な文字列構築のための追加バッファ機能を提供：
 * - 動的メモリ管理
 * - 文字列の追加
 * - メモリの解放
 * 
 * 画面描画時に大量の文字列を効率的に構築するために使用
 */

#include "kiloe.h"

/**
 * 追加バッファに文字列を追加
 * 必要に応じてバッファサイズを拡張
 */
void abAppend(struct abuf *ab, const char *s, int len) {
    // 新しいサイズでメモリを再割り当て
    char *new = realloc(ab->b, ab->len + len);
    if (new == NULL) return;  // メモリ不足の場合は何もしない
    
    // 新しい文字列を末尾に追加
    memcpy(&new[ab->len], s, len);
    ab->b = new;
    ab->len += len;
}

/**
 * 追加バッファのメモリを解放
 */
void abFree(struct abuf *ab) {
    free(ab->b);
    ab->b = NULL;
    ab->len = 0;
}