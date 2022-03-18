PREFIX = /usr/local
ARCH = h8300-elf
BINDIR = $(PREFIX)/bin
ADDNAME = $(ARCH)-

# 利用するツール類の指定
AR = $(BINDIR)/$(ADDNAME)ar
AS = $(BINDIR)/$(ADDNAME)as
CC = $(BINDIR)/$(ADDNAME)gcc
LD = $(BINDIR)/$(ADDNAME)ld
NM = $(BINDIR)/$(ADDNAME)nm
OBJCOPY = $(BINDIR)/$(ADDNAME)objcopy
OBJDUMP = $(BINDIR)/$(ADDNAME)objdump
RANLIB = $(BINDIR)/$(ADDNAME)ranlib
STRIP = $(BINDIR)/$(ADDNAME)strip

# コンパイルするソース軍
OBJS = startup.o main.o interrupt.o
OBJS += lib.o serial.o

# 生成する実行形式のファイル名
TARGET = kozos

# コンパイルオプション
# すべての警告を表示，H8/300H用のコードを出力，システムのヘッダフィアルを利用しない，システムのライブラリを利用しない，コンパイラのビルトイン関数を利用しない
CFLAGS = -Wall -mh -nostdinc -nostdlib -fno-builtin
# intを32bitにする
#CFLAGS += -mint32 # intを32ビットにすると掛け算と割り算ができなくなる
# ヘッダファイルの検索先を指定する
CFLAGS += -I.
# デバッガを利用する
#CFLAGS += -g
# サイズ節約の最適化を行う
CFLAGS += -Os
# ブートローダー特有の処理やOS特有の処理を入れたくなったときに，　自身がブートローダーなのかOSなのか判別できるように定義
CFLAGS += -DKOZOS

# リンクオプション
# 全て静的リンクする，リンカスクリプトを指定する，ライブラリの検索先を指定する
LFLAGS = -static -T ld.scr -L.

# ビルドルール
.SUFFIXES: .c .o
.SUFFIXES: .s .o
.SUFFIXES: .S .o

all:	$(TARGET)

# 実行形式の生成ルール
$(TARGET):	$(OBJS)
			$(CC) $(OBJS) -o $(TARGET) $(CFLAGS) $(LFLAGS)
			cp $(TARGET) $(TARGET).elf
			$(STRIP) $(TARGET)

# *.cファイルのコンパイルルール
.c.o:		$<
			$(CC) -c $(CFLAGS) $<

# アセンブラファイルのアセンブルルール
.s.o:		$<
			$(CC) -c $(CFLAGS) $<

.S.o:		$<
			$(CC) -c $(CFLAGS) $<

# ファイルの削除
clean:
		rm -f $(OBJS) $(TARGET) $(TARGET).elf