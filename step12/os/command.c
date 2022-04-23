#include "defines.h"
#include "kozos.h"
#include "consdrv.h"
#include "lib.h"

// コンソールドライバの使用開始をコンソールドライバに依頼
// コンソールの初期化を依頼する
static void send_use(int index)
{
	char *p;
	// コマンド通知用の領域を獲得
	p = kz_kmalloc(3);
	p[0] = '0';
	// 初期化コマンドを設定
	p[1] = CONSDRV_CMD_USE;
	p[2] = '0' + index;
	// コンソールドライバスレッドに送信
	kz_send(MSGBOX_ID_CONSOUTPUT, 3, p);
}

// コンソールへの文字列出力をコンソールドライバに依頼する
// 文字列の出力を依頼
static void send_write(char *str)
{
	char *p;
	int len;
	len = strlen(str);
	// コマンド通知用の領域を獲得
	p = kz_kmalloc(len + 2);
	// コンソールドライバスレッドに対するコマンド発行の実体
	// 実際にはコマンドとパラメータをメモリ上に詰め込み、メッセージでコンソールドライバスレッドに送信するだけ
	p[0] = '0';				  // コマンド
	p[1] = CONSDRV_CMD_WRITE; // パラメータ
	memcpy(&p[2], str, len);
	// コンソールドライバスレッドに送信
	kz_send(MSGBOX_ID_CONSOUTPUT, len + 2, p);
}

int command_main(int arvc, char *argv[])
{
	char *p;
	int size;

	// コンソールドライバスレッドにコンソールの初期化を依頼する
	send_use(SERIAL_DEFAULT_DEVICE);

	while (1)
	{
		send_write("command> ");

		// コンソールドライバスレッドから受信文字列を受け取る
		// -> pに入る
		kz_recv(MSGBOX_ID_CONSINPUT, &size, &p);
		p[size] = '\0';

		// echoコマンドの処理
		if (!strncmp(p, "echo", 4))
		{
			// echoに続く文字列を出力
			// コンソールドライバスレッドにメッセージが送信される
			send_write(p + 4);
			send_write("\n");
		}
		else
		{
			send_write("unknown.\n");
		}

		// メッセージにより受信した領域（送信元で獲得されたもの）を解放する
		kz_kmfree(p);
	}

	return 0;
}