#include "defines.h"
#include "kozos.h"
#include "interrupt.h"
#include "lib.h"

// kz_thread_id_t test09_1_id;
// kz_thread_id_t test09_2_id;
// kz_thread_id_t test09_3_id;

// どこから？
// 『kozos.c』の『thread_init関数』（init.func）
// スレッドとして生成されて実行される関数は，すべて『thread_init関数』（init.func）
// 初期スレッドである『start_threads関数』はアイドルスレッドとなる
// システムタスクとユーザタスクの起動
static int start_threads(int argc, char *argv[])
{
	// ここで割込み有効にしないと，kz_runを実行できないんじゃないの？

	// kz_run(test08_1_main, "command", 0x100, 0, NULL);
	//       			メイン関数　　　　名前　　　優先番号　　スタックサイズ    argc   argv
	// test09_1_id = kz_run(test09_1_main, "test09_1", 1, 0x100, 0, NULL);
	// test09_2_id = kz_run(test09_2_main, "test09_2", 2, 0x100, 0, NULL);
	// test09_3_id = kz_run(test09_3_main, "test09_3", 3, 0x100, 0, NULL);
	// kz_run(test10_1_main, "test10_1", 1, 0x100, 0, NULL);
	// kz_run(test11_1_main, "test11_1", 1, 0x100, 0, NULL);
	// kz_run(test11_2_main, "test11_2", 2, 0x100, 0, NULL);
	// コンソールドライバスレッドを起動
	kz_run(consdrv_main, "consdrv", 1, 0x200, 0, NULL);
	// コマンドスレッドを起動
	kz_run(command_main, "command", 8, 0x200, 0, NULL);

	// start_threadsスレッドの優先順位を下げて，アイドルスレッドに移行する（優先順位を最低にする）
	kz_chpri(15);

	// スレッドは割込み無効で動作する．
	// 明示的にシステムコール（trap命令）を呼び出さない限り，割込み命令が入ることはない．．それはそれでどうなの？？

	// 割り込み有効
	// 有効にしてアイドルスレッドに切り替える
	INTR_ENABLE;

	// このスレッドを省電力で永遠に動作
	// 初期スレッドがここで，アイドルスレッドに移行するような構成にしている (スレッドをまとめることで，メモリのスタック領域を節約)
	while (1)
	{
		// 省電力モードに移行
		asm volatile("sleep");
	}

	return 0;
}

// どこから？
// 『bootload/main.c』の『main関数（runコマンド内のf()）』
int main(void)
{
	INTR_DISABLE;

	puts("kozos boot succeed!\n");

	// "start"という名前のスレッドが，start_threads()をメイン関数として動作開始
	//       メイン関数　　　　名前　　　スタックサイズ    argc   argv
	kz_start(start_threads, "idle", 0, 0x100, 0, NULL);
	// 『start_thrads関数』は割込み禁止として，実行させたい．

	return 0;
}
