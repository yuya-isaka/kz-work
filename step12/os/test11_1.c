#include "defines.h"
#include "kozos.h"
#include "lib.h"

// どこから？
// 『kozos.c』の『thread_init関数』のinit.func()
int test11_1_main(int argc, char *argv[])
{
	char *p;
	int size;

	puts("test11_1 started.\n");

	// 静的領域をメッセージで受信
	puts("test11_1 recv in.\n");
	// MSGBOX1で受信
	kz_recv(MSGBOX_ID_MSGBOX1, &size, &p);
	puts("test11_1 recv out.\n");
	// 受信結果
	puts(p);

	// 動的領域をメッセージで受信
	puts("test11_1 recv in.\n");
	// MSGBOX1で受信
	kz_recv(MSGBOX_ID_MSGBOX1, &size, &p);
	puts("test11_1 recv out.\n");
	// 受信結果
	puts(p);
	// 受信した領域を解放
	kz_kmfree(p);

	// 静的領域をメッセージで送信
	puts("test11_1 send in.\n");
	// MSGBOX2に送信
	kz_send(MSGBOX_ID_MSGBOX2, 15, "static memory\n");
	puts("test11_1 send out.\n");

	// 動的領域をメッセージで送信
	p = kz_kmalloc(18);
	strcpy(p, "allocated memory\n");
	puts("test11_1 send in.\n");
	// MSGBOX2に送信
	kz_send(MSGBOX_ID_MSGBOX2, 18, p);
	puts("test11_1 send out\n");
	// メモリ解放は受信側で実行

	puts("test11_1 exit.\n");

	return 0;
}
