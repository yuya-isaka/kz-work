// main()関数
#include "defines.h"
#include "serial.h"
#include "xmodem.h"
#include "lib.h"
#include "elf.h"

static int init(void)
{
	/*
	リンカスクリプトで定義されているシンボル
	  ROM
		text_start
		etext

		rodata_start
		erodata

	  RAM
		data_start
		edata

		bss_start
		ebss
	*/
	extern int erodata, data_start, edata, bss_start, ebss;

	// データ領域とBSS領域を初期化（　この処理以降でないとグローバル変数が初期化されていない ので　利用するのはこの後）
	// シンボルのアドレスをポインタ参照することで, C言語のプログラム側から扱える
	memcpy(&data_start, &erodata, (long)&edata - (long)&data_start); // ROMの.dataをRAMの.dataに移動, VA!=PA対策
	memset(&bss_start, 0, (long)&ebss - (long)&bss_start);			 // ゼロクリア

	// シリアルの初期化
	// SCIのレジスタをデータ長を8ビット，ストップビット長を1,パリティ無しで初期化
	// OSがインタフェースを用意する = デバイスドライバを書く
	serial_init(SERIAL_DEFAULT_DEVICE);

	return 0;
}

// メモリの16進数ダンプ
static int dump(char *buf, long size)
{
	long i;

	if (size < 0)
	{
		puts("no data.\n");
		return -1;
	}

	for (i = 0; i < size; i++)
	{
		// 16進数文字を２個出力
		// 1バイト出力
		// buf[i] は　char 一個分，　charは１バイトなので，　char一個分は16進数文字２個分で正しい
		// なのでbuf[i]で２と指定する
		putxval(buf[i], 2);

		// i % 16 と一緒
		//                                                  ↓ この部分
		// 23 69 66 6e 64 65 66 20  5f 44 45 46 49 4e 45 53
		// 5f 48 5f 49 4e 43 4c 55  44 45 44 5f 0a 23 64 65
		if ((i & 0xf) == 15)
		{
			puts("\n");
		}
		else
		{
			// i % 16と一緒
			//                         ↓ この部分
			// 23 69 66 6e 64 65 66 20  5f 44 45 46 49 4e 45 53
			if ((i & 0xf) == 7)
				puts(" ");
			puts(" ");
		}
	}
	puts("\n");

	return 0; // 正常に終了
}

static void wait()
{
	volatile long i;
	for (i = 0; i < 300000; i++)
		;
}

int global_data = 0x10;		   // -> .data
int global_bss;				   // -> .bss
static int static_data = 0x20; // -> .data
static int static_bss;		   // -> .bss

static void printval(void)
{
	puts("global_data = ");
	putxval(global_data, 0);
	puts("\n");

	puts("global_bss = ");
	putxval(global_bss, 0);
	puts("\n");

	puts("static_data = ");
	putxval(static_data, 0);
	puts("\n");

	puts("static_bss = ");
	putxval(static_bss, 0);
	puts("\n");
}

int main(void)
{
	static char buf[16];
	static long size = -1;
	static unsigned char *loadbuf = NULL;
	// リンカスクリプトで定義されているバッファ
	// バッファ領域を指すシンボル
	// ロードされてここに入れたプログラムは，,後でRAM上にELF形式のセグメントの指示する通りに展開されて実行される
	// -> なのでこれは一時的に置いておくにすぎない
	// 普通はRAMの先頭からプログラムを始めるものだから，　ロードは後ろの方に入れておく
	extern int buffer_start;

	char *entry_point;
	void (*f)(void); // 関数へのポインタ

	// initした後にグローバル変数は使える
	init();

	for (;;)
	{
		puts("kzload> ");
		// 端末変換した結果をbufに格納
		gets(buf);

		// load
		if (!strcmp(buf, "load"))
		{
			// ロードする場所
			loadbuf = (char *)(&buffer_start);

			size = xmodem_recv(loadbuf);
			// 転送アプリ(xmodem)が終了し， mainに処理が戻るまで待ち合わせる．
			wait();

			if (size < 0)
			{
				puts("\nXMODEM receive error!\n");
			}
			else
			{
				puts("\nXMODEM receive succeeded.\n");
			}
		}
		// dump
		else if (!strcmp(buf, "dump"))
		{
			puts("size: ");
			putxval(size, 0); // 1でも良い気がする．．また実験しよう
			puts("\n");
			dump(loadbuf, size);
		}
		// run (ELF形式ファイルの実行)
		else if (!strcmp(buf, "run"))
		{
			entry_point = elf_load(loadbuf); // ロードしてきたプログラムのエントリーポイントget
			if (!entry_point) // ここではアドレス自体に興味がある．アドレスの先のプログラムはこの後実行される処理がある
			{
				puts("run error!\n");
			}
			else
			{
				puts("starting from entry point: ");
				putxval((unsigned long)entry_point, 0);
				puts("\n");
				f = (void (*)(void))entry_point; // 関数へのポインタにキャストして代入
				f(); // ロードしたプログラムを実行（処理を渡す）
				// ここにはもう戻ってこない．．そして誰もいなくなった．．（ロードしたプログラムからリターンしたなら別）
			}
		}
		// 該当なし
		else
		{
			puts("unknown.\n");
		}
	}

	puts("Hello World!\n");

	putxval(0x10, 0);
	puts("\n");

	putxval(0xffff, 0);
	puts("\n");

	printval();

	puts("overwrite variables.\n");
	global_data = 0x20;
	global_bss = 0x30;
	static_data = 0x40;
	static_bss = 0x50;
	printval();

	while (1)
		;

	return 0;
}
