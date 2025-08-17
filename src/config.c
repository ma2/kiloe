/**
 * config.c - 設定ファイル管理機能
 * 
 * 設定ファイルの読み込み、デフォルト設定の初期化、
 * key=value形式の設定パーサーを提供する
 */

#include "kiloe.h"

/**
 * デフォルト設定を初期化
 */
void initDefaultConfig() {
    // エディタ設定
    Config.tab_stop = 8;
    Config.quit_times = 3;
    Config.show_line_numbers = 0;
    
    // 表示設定
    strcpy(Config.welcome_message, "Kilo editor -- version 0.0.1");
    Config.status_timeout = 5;
    
    // カラー設定
    Config.color_comment = 36;      // シアン
    Config.color_keyword1 = 33;     // 黄色
    Config.color_keyword2 = 32;     // 緑
    Config.color_string = 35;       // マゼンタ
    Config.color_number = 31;       // 赤
    Config.color_match = 34;        // 青
}

/**
 * 文字列をtrueまたはfalseとして解釈
 */
static int parseBool(const char *value) {
    if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0) {
        return 1;
    }
    return 0;
}

/**
 * 設定ファイルを読み込み
 * key=value形式の設定ファイルをパースして設定を適用する
 */
int loadConfig(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return -1;
    
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        // コメント行と空行をスキップ
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        
        // = で分割
        char *eq = strchr(line, '=');
        if (!eq) continue;
        
        *eq = '\0';
        char *key = line;
        char *value = eq + 1;
        
        // 改行文字を削除
        char *newline = strchr(value, '\n');
        if (newline) *newline = '\0';
        newline = strchr(value, '\r');
        if (newline) *newline = '\0';
        
        // 先頭と末尾の空白を削除
        while (*key == ' ' || *key == '\t') key++;
        while (*value == ' ' || *value == '\t') value++;
        
        // キーの末尾空白を削除
        char *key_end = key + strlen(key) - 1;
        while (key_end > key && (*key_end == ' ' || *key_end == '\t')) {
            *key_end = '\0';
            key_end--;
        }
        
        // 設定値を適用
        if (strcmp(key, "tab_stop") == 0) {
            Config.tab_stop = atoi(value);
        } else if (strcmp(key, "quit_times") == 0) {
            Config.quit_times = atoi(value);
        } else if (strcmp(key, "show_line_numbers") == 0) {
            Config.show_line_numbers = parseBool(value);
        } else if (strcmp(key, "welcome_message") == 0) {
            strncpy(Config.welcome_message, value, sizeof(Config.welcome_message) - 1);
            Config.welcome_message[sizeof(Config.welcome_message) - 1] = '\0';
        } else if (strcmp(key, "status_timeout") == 0) {
            Config.status_timeout = atoi(value);
        } else if (strcmp(key, "color_comment") == 0) {
            Config.color_comment = atoi(value);
        } else if (strcmp(key, "color_keyword1") == 0) {
            Config.color_keyword1 = atoi(value);
        } else if (strcmp(key, "color_keyword2") == 0) {
            Config.color_keyword2 = atoi(value);
        } else if (strcmp(key, "color_string") == 0) {
            Config.color_string = atoi(value);
        } else if (strcmp(key, "color_number") == 0) {
            Config.color_number = atoi(value);
        } else if (strcmp(key, "color_match") == 0) {
            Config.color_match = atoi(value);
        }
    }
    
    fclose(fp);
    return 0;
}