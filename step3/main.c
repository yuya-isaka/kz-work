// main()関数
#include "defines.h"
#include "serial.h"
#include "lib.h"

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
	memcpy(&data_start, &erodata, (long)&edata - (long)&data_start); // ROMの.dataをRAMの.dataに移動
	memset(&bss_start, 0, (long)&ebss - (long)&bss_start);			 // ゼロクリア

	// シリアルの初期化
	// SCIのレジスタをデータ長を8ビット，ストップビット長を1,パリティ無しで初期化
	// OSがインタフェースを用意する = デバイスドライバを書く
	serial_init(SERIAL_DEFAULT_DEVICE);

	return 0;
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

// ファームウェア生成
int main(void)
{
	init();

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
