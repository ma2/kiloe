/**
 * file.c - ファイル入出力機能
 * 
 * ファイルの読み込み・保存機能を提供：
 * - テキストファイルの読み込み
 * - エディタ内容のファイル保存
 * - ファイル名の管理
 */

#include "kiloe.h"

/**
 * 編集中のテキストを1つの文字列に変換
 * 各行を改行文字で区切った文字列を作成
 */
char *editorRowsToString(int *buflen) {
    int totlen = 0;
    int j;
    
    // 全行の合計文字数を計算（改行文字分も含む）
    for (j = 0; j < E.numrows; j++) {
        totlen += E.row[j].size + 1;
    }
    *buflen = totlen;

    // バッファを確保して各行をコピー
    char *buf = malloc(totlen);
    char *p = buf;
    for (j = 0; j < E.numrows; j++) {
        memcpy(p, E.row[j].chars, E.row[j].size);
        p += E.row[j].size;
        *p = '\n';
        p++;
    }

    return buf;
}

/**
 * ファイルを読み込んで編集バッファにセット
 */
void editorOpen(char *filename) {
    free(E.filename);
    E.filename = strdup(filename);

    // ファイル拡張子からシンタックスハイライトを選択
    editorSelectSyntaxHighlight();

    FILE *fp = fopen(filename, "r");
    if (!fp) die("fopen");

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    
    // ファイルを1行ずつ読み込み
    while ((linelen = getline(&line, &linecap, fp)) != -1) {
        // 行末の改行文字を除去
        while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) {
            linelen--;
        }
        editorInsertRow(E.numrows, line, linelen);
    }
    
    free(line);
    fclose(fp);
    E.dirty = 0;  // 読み込み直後は変更なし
}

/**
 * エディタ内容をファイルに保存
 */
void editorSave() {
    if (E.filename == NULL) {
        // ファイル名が未設定の場合はプロンプトで入力
        E.filename = editorPrompt("Save as: %s (ESC to cancel)", NULL);
        if (E.filename == NULL) {
            editorSetStatusMessage("Save aborted");
            return;
        }
        editorSelectSyntaxHighlight();
    }

    int len;
    char *buf = editorRowsToString(&len);
    int fd = open(E.filename, O_RDWR | O_CREAT, 0644);

    if (fd != -1) {
        if (ftruncate(fd, len) != -1) {
            if (write(fd, buf, len) != -1) {
                close(fd);
                free(buf);
                E.dirty = 0;
                editorSetStatusMessage("%d bytes written to disk", len);
                return;
            }
        }
        close(fd);
    }

    free(buf);
    editorSetStatusMessage("Can't save! I/O error: %s", strerror(errno));
}