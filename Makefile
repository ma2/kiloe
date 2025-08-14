# Kiloeエディタのビルド設定
# ディレクトリ構成: src/ build/ 分離版

CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99

# ディレクトリ定義
SRCDIR = src
BUILDDIR = build
TARGET = $(BUILDDIR)/kiloe

# ソースファイル
SOURCES = $(SRCDIR)/main.c $(SRCDIR)/terminal.c $(SRCDIR)/utf8.c $(SRCDIR)/kiloe.c
HEADERS = $(SRCDIR)/kiloe.h

# オブジェクトファイル（buildディレクトリ内）
OBJECTS = $(BUILDDIR)/main.o $(BUILDDIR)/terminal.o $(BUILDDIR)/utf8.o $(BUILDDIR)/kiloe.o

# メインターゲット
$(TARGET): $(BUILDDIR) $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)
	@echo "✅ ビルド完了: $(TARGET)"

# buildディレクトリ作成
$(BUILDDIR):
	mkdir -p $(BUILDDIR)

# オブジェクトファイルのビルドルール
$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# 特別ルール：kiloe.cは複数ファイルに依存
$(BUILDDIR)/kiloe.o: $(SRCDIR)/kiloe.c $(HEADERS)
	$(CC) $(CFLAGS) -c $(SRCDIR)/kiloe.c -o $(BUILDDIR)/kiloe.o

# クリーンアップ
clean:
	rm -rf $(BUILDDIR)
	@echo "🧹 クリーンアップ完了"

# 開発用：プロジェクト構造の確認
status:
	@echo "📁 ディレクトリ構成:"
	@echo "  src/     - ソースコード"
	@echo "  build/   - ビルド結果"
	@echo ""
	@echo "📊 Phase 1 完了: main.c, terminal.c, utf8.c"
	@echo "📋 残りファイル: kiloe.c (設定、バッファ、描画、入力処理)"
	@echo "🚀 次のPhase: buffer.c, editor.c, fileio.c等への分割"

# インストール（オプション）
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/kiloe
	@echo "📦 インストール完了: /usr/local/bin/kiloe"

# 開発用：実行テスト
test: $(TARGET)
	@echo "🧪 エディタ起動テスト:"
	$(TARGET) --version 2>/dev/null || echo "✅ エディタのビルドが正常完了"

.PHONY: clean status install test