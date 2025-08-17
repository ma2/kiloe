/**
 * utf8.c - UTF-8文字エンコーディング処理
 * 
 * マルチバイト文字の境界検出、文字幅計算など
 * UTF-8テキストを正しく扱うためのヘルパー関数群
 */

#include "kiloe.h"

/**
 * is_utf8_continuation - UTF-8の継続バイトかチェック
 * @c: チェック対象のバイト
 * 
 * UTF-8の継続バイトは 10xxxxxx (0x80-0xBF) の形式
 * 
 * @return: 継続バイトの場合1、それ以外は0
 */
int is_utf8_continuation(unsigned char c) {
    return (c & 0xC0) == 0x80;
}

/**
 * move_to_next_char - 次のUTF-8文字の開始位置へ移動
 * @str: 対象文字列
 * @pos: 現在位置
 * @max: 文字列の最大長
 * 
 * 現在位置から次の文字の開始位置まで移動する。
 * UTF-8の継続バイトをスキップして文字境界を見つける。
 * 
 * @return: 次の文字の開始位置（インデックス）
 */
int move_to_next_char(char *str, int pos, int max) {
    if (pos >= max) return max;
    
    // 1バイト進める
    pos++;
    
    // 継続バイトをスキップして文字の先頭を探す
    while (pos < max && is_utf8_continuation((unsigned char)str[pos])) {
        pos++;
    }
    
    return pos;
}

/**
 * move_to_prev_char - 前のUTF-8文字の開始位置へ移動
 * @str: 対象文字列
 * @pos: 現在位置
 * 
 * 現在位置から前の文字の開始位置まで移動する。
 * UTF-8の継続バイトを逆方向にスキップして文字境界を見つける。
 * 
 * @return: 前の文字の開始位置（インデックス）
 */
int move_to_prev_char(char *str, int pos) {
    if (pos <= 0) return 0;
    
    // 1バイト戻る
    pos--;
    
    // 継続バイトを逆方向にスキップして文字の先頭を探す
    while (pos > 0 && is_utf8_continuation((unsigned char)str[pos])) {
        pos--;
    }
    
    return pos;
}

/**
 * utf8_char_len - UTF-8文字のバイト長を取得
 * @c: UTF-8文字の最初のバイト
 * 
 * UTF-8エンコーディングの最初のバイトから文字のバイト長を判定
 * - 0xxxxxxx: 1バイト（ASCII）
 * - 110xxxxx: 2バイト
 * - 1110xxxx: 3バイト（日本語文字など）
 * - 11110xxx: 4バイト（絵文字など）
 * 
 * @return: 文字のバイト数（1-4）
 */
int utf8_char_len(unsigned char c) {
    if (c < 0x80) return 1;         // 0xxxxxxx
    if ((c & 0xE0) == 0xC0) return 2; // 110xxxxx
    if ((c & 0xF0) == 0xE0) return 3; // 1110xxxx
    if ((c & 0xF8) == 0xF0) return 4; // 11110xxx
    return 1; // 不正なバイト、安全のため1として扱う
}

/**
 * get_char_width - UTF-8文字の表示幅を取得
 * @str: 対象文字列
 * @pos: 文字の開始位置
 * 
 * UTF-8文字の表示幅を計算する。
 * - ASCII文字（1バイト）: 幅1
 * - 日本語文字（3バイト）: 幅2（全角）
 * - その他: 幅1（簡易実装）
 * 
 * 注意: 完全な実装にはwcwidth()やEast Asian Width属性の
 *       考慮が必要だが、ここでは簡易実装としている
 * 
 * @return: 文字の表示幅（1または2）
 */
int get_char_width(char *str, int pos) {
    unsigned char c = (unsigned char)str[pos];
    
    // ASCII文字（0x00-0x7F）は1幅
    if (c < 0x80) {
        return 1;
    }
    
    // 3バイトUTF-8文字（0xE0-0xEF で始まる）
    // 多くの日本語文字（ひらがな、カタカナ、漢字）は3バイトで幅2
    if ((c & 0xF0) == 0xE0) {
        return 2;
    }
    
    // 2バイトUTF-8（0xC0-0xDF）: 主にラテン拡張文字、幅1
    // 4バイトUTF-8（0xF0-0xF7）: 絵文字など、簡易実装では幅1
    return 1;
}