/**
 * syntax.c - シンタックスハイライト機能
 * 
 * ソースコードの構文解析、キーワード・コメント・文字列・数値の
 * ハイライト処理、ファイル種別の自動判定を行う
 */

#include "kiloe.h"

/**
 * セパレータ文字の判定
 * 単語境界を判定するために使用される
 */
int is_separator(int c) {
    return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}

/**
 * 行のシンタックスハイライトを更新
 * 各行のテキストを解析してキーワード、コメント、文字列、数値をハイライト
 */
void editorUpdateSyntax(erow *row) {
    // ハイライト情報配列をrender配列と同じサイズに再割り当て
    row->hl = realloc(row->hl, row->rsize);
    // 全体をHL_NORMAL（通常テキスト）で初期化
    memset(row->hl, HL_NORMAL, row->rsize);

    // シンタックス定義がない場合は処理終了
    if (E.syntax == NULL) return;

    // シンタックス定義から各種設定を取得
    char **keywords = E.syntax->keywords;
    char *scs = E.syntax->singleline_comment_start;    // 単行コメント開始文字
    char *mcs = E.syntax->multiline_comment_start;     // 複数行コメント開始文字
    char *mce = E.syntax->multiline_comment_end;       // 複数行コメント終了文字

    int scs_len = scs ? strlen(scs) : 0;
    int mcs_len = mcs ? strlen(mcs) : 0;
    int mce_len = mce ? strlen(mce) : 0;

    // 解析状態の初期化
    int prev_sep = 1;      // 直前がセパレータかどうか（行頭はセパレータ扱い）
    int in_string = 0;     // 文字列リテラル内かどうか（0=文字列外、'"'=ダブルクォート内、'\''=シングルクォート内）
    // 前行から続く複数行コメント内かどうかを判定
    int in_comment = (row->idx > 0 && E.row[row->idx - 1].hl_open_comment);

    int i = 0;
    while (i < row->rsize) {
        char c = row->render[i];
        unsigned char prev_hl = (i > 0) ? row->hl[i - 1] : HL_NORMAL;

        // 単行コメントの処理
        if (scs_len && !in_string && !in_comment) {
            if (!strncmp(&row->render[i], scs, scs_len)) {
                // 単行コメント開始文字が見つかった場合、行末まで全てコメント
                memset(&row->hl[i], HL_COMMENT, row->rsize - i);
                break;
            }
        }

        // 複数行コメントの処理
        if (mcs_len && mce_len && !in_string) {
            if (in_comment) {
                // 既に複数行コメント内の場合
                row->hl[i] = HL_MLCOMMENT;
                // 複数行コメント終了文字をチェック
                if (!strncmp(&row->render[i], mce, mce_len)) {
                    memset(&row->hl[i], HL_MLCOMMENT, mce_len);
                    i += mce_len;
                    in_comment = 0;
                    prev_sep = 1;
                    continue;
                }
            } else if (!strncmp(&row->render[i], mcs, mcs_len)) {
                // 複数行コメント開始文字が見つかった場合
                memset(&row->hl[i], HL_MLCOMMENT, mcs_len);
                i += mcs_len;
                in_comment = 1;
                continue;
            }
        }

        // 文字列リテラルの処理
        if (E.syntax->flags & HL_HIGHLIGHT_STRINGS) {
            if (in_string) {
                // 文字列リテラル内の場合
                row->hl[i] = HL_STRING;
                // エスケープシーケンスの処理
                if (c == '\\' && i + 1 < row->rsize) {
                    row->hl[i + 1] = HL_STRING;
                    i += 2;
                    continue;
                }
                // 文字列終了文字に達した場合
                if (c == in_string) in_string = 0;
                i++;
                prev_sep = 1;
                continue;
            } else {
                // 文字列開始文字（"または'）をチェック
                if (c == '"' || c == '\'') {
                    in_string = c;
                    row->hl[i] = HL_STRING;
                    i++;
                    continue;
                }
            }
        }

        // 数値リテラルの処理
        if (E.syntax->flags & HL_HIGHLIGHT_NUMBERS) {
            if ((isdigit(c) && (prev_sep || prev_hl == HL_NUMBER)) || 
                (c == '.' && prev_hl == HL_NUMBER)) {
                // 数字かつ直前がセパレータまたは数値の場合、または
                // ドットかつ直前が数値の場合（小数点）
                row->hl[i] = HL_NUMBER;
                i++;
                prev_sep = 0;
                continue;
            }
        }

        // キーワードの処理
        if (prev_sep) {
            // 直前がセパレータの場合のみキーワードチェック
            int j;
            for (j = 0; keywords[j]; j++) {
                int klen = strlen(keywords[j]);
                // キーワードの末尾に|がある場合はキーワード2（型名など）
                int kw2 = keywords[j][klen - 1] == '|';
                if (kw2) klen--;

                // キーワードとのマッチングをチェック
                if (!strncmp(&row->render[i], keywords[j], klen) && 
                    is_separator(row->render[i + klen])) {
                    memset(&row->hl[i], kw2 ? HL_KEYWORD2 : HL_KEYWORD1, klen);
                    i += klen;
                    break;
                }
            }
            // キーワードが見つかった場合は次の文字へ
            if (keywords[j] != NULL) {
                prev_sep = 0;
                continue;
            }
        }

        // 現在の文字がセパレータかどうかを更新
        prev_sep = is_separator(c);
        i++;
    }

    // 複数行コメント状態の変化をチェックし、次行に影響する場合は更新
    int changed = (row->hl_open_comment != in_comment);
    row->hl_open_comment = in_comment;
    if (changed && row->idx + 1 < E.numrows) {
        editorUpdateSyntax(&E.row[row->idx + 1]);
    }
}

/**
 * シンタックスハイライト種別をANSI色コードに変換
 */
int editorSyntaxToColor(int hl) {
    switch (hl) {
        case HL_COMMENT:
        case HL_MLCOMMENT: return Config.color_comment;
        case HL_KEYWORD1: return Config.color_keyword1;
        case HL_KEYWORD2: return Config.color_keyword2;
        case HL_STRING: return Config.color_string;
        case HL_NUMBER: return Config.color_number;
        case HL_MATCH: return Config.color_match;
        default: return 37;  // 白色
    }
}

/**
 * ファイル名からシンタックスハイライト設定を選択
 * ファイル拡張子やファイル名パターンに基づいて自動判定
 */
void editorSelectSyntaxHighlight() {
    E.syntax = NULL;
    if (E.filename == NULL) return;

    // ファイル拡張子を取得
    char *ext = strrchr(E.filename, '.');
    
    // HLDB_ENTRIESを一度だけ計算して保存
    int hldb_entries = getHLDBEntries();

    // 各シンタックス定義を順番にチェック
    for (int j = 0; j < hldb_entries; j++) {
        struct editorSyntax *s = &HLDB[j];
        int i = 0;
        
        
        // 各ファイルマッチパターンをチェック
        while (s->filematch[i]) {
            int is_ext = (s->filematch[i][0] == '.');
            
            // 拡張子マッチまたはファイル名パターンマッチをチェック
            if ((is_ext && ext && !strcmp(ext, s->filematch[i])) || 
                (!is_ext && strstr(E.filename, s->filematch[i]))) {
                
                // マッチした場合、シンタックス定義を設定
                E.syntax = s;
                
                

                // 全行のシンタックスハイライトを更新
                int filerow;
                for (filerow = 0; filerow < E.numrows; filerow++) {
                    editorUpdateSyntax(&E.row[filerow]);
                }

                return;
            }
            i++;
        }
    }
    
}