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

H8WRITE = ../tools/PackageFiles/src/kz_h8write

#シリアル接続先
# Linux(USB): /dev/ttyUSBx
H8WRITE_SERDEV = /dev/ttyUSB0

# コンパイルするソース軍
OBJS = vector.o startup.o main.o
OBJS += lib.o serial.o

# 生成する実行形式のファイル名
TARGET = kzload

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
CFLAGS += -DKZLOAD

# リンクオプション
# 全て静的リンクする，リンカスクリプトを指定する，ライブラリの検索先を指定する
LFLAGS = -static -T ld.scr -L.

.SUFFIXES: .c .o
.SUFFIXES: .s .o

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

# モトローラSレコード・フォーマットへの変換ルール
$(TARGET).mot: 	$(TARGET)
				$(OBJCOPY) -O srec $(TARGET) $(TARGET).mot

image:	$(TARGET).mot

# フラッシュROMへの転送
write:	$(TARGET).mot
		$(H8WRITE) -3069 -f20 $(TARGET).mot $(H8WRITE_SERDEV)

# ファイルの削除
clean:
		rm -f $(OBJS) $(TARGET) $(TARGET).elf $(TARGET).mot