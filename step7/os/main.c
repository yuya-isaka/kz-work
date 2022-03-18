// OS

#include "defines.h"
#include "serial.h"
#include "lib.h"

int main(void)
{
	static char buf[32];

	puts("Hello World!\n");

	for (;;)
	{
		puts("> ");
		gets(buf); // コンソールからの１行入力

		if (!strncmp(buf, "echo", 4)) // echoコマンド
		{
			puts(buf + 4); // 4文字のechoの後続の文字列を表示
			puts("\n");
		}
		else if (!strcmp(buf, "exit")) // exitコマンド
		{
			break;
		}
		else // 不明
		{
			puts("unknown.\n");
		}
	}

	return 0;
}

/*
下記の処理が不要になり， bootloadと比べてシンプル (init()が消えた)

	・データ領域のコピー ... 初めからRAM上で動作するから「VA!=PA」の対策が不要

	・BSS領域のクリア ... プログラムのロード時にブート・ローダがBSS領域のクリアを行なってくれるから対策不要　（elf_load_program）

	・シリアル・デバイスの初期化 ... ブート・ローダが実行してくれるから対策不要
*/
