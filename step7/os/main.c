// OS

#include "defines.h"
#include "intr.h"
#include "interrupt.h"
#include "serial.h"
#include "lib.h"

// シリアル割込みのハンドラ
static void intr(softvec_type_t type, unsigned long sp)
{
	int c;
	static char buf[32];
	static int len;

	c = getc();

	if (c != '\n') // 受信したのが改行でないならバッファに保存
	{
		buf[len++] = c;
	}
	else // 改行を受信したらコマンド処理
	{
		buf[len++] = '\0';
		if (!strncmp(buf, "echo", 4))
		{
			puts(buf + 4);
			puts("\n");
		}
		else
		{
			puts("unknown.\n");
		}
		puts("> ");
		len = 0;
	}
}

int main(void)
{
	// 割込み無効化 (初期化するときは無効化しておく)
	// ブートローダ側で，割り込み無効の状態でOSを起動しているからこれはいらないが，割り込み関連の設定は一応
	// INTR_DISABLEとINTR_ENABLEで挟むようにする
	INTR_DISABLE;

	puts("kozos boot succeed!\n");

	// ソフトウェア割込みベクタにシリアル割込みのハンドラを設定
	softvec_setintr(SOFTVEC_TYPE_SERINTR, intr);

	// シリアル受信割込みを有効化
	sefal_intr_recv_enable(SERIAL_DEFAULT_DEVICE);

	puts("> ");

	// 割込み有効化
	INTR_ENABLE;

	// この後はコンソールで１文字入力するたびに割り込み処理が入って，処理が続けられる

	for (;;)
	{
		asm volatile("sleep"); // 省電力モード（組み込みプロセッサは持っている）
	}
	return 0;
}