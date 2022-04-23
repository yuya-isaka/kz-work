// コンソールデバイスドライバ
// 今回は表示するコンソールの数は１
// それぞれのスレッドがコンソールを利用したい→コンソールを管理しているスレッドに連絡→使ってよかったら使う
// コンソールを管理しているスレッドは，そこからサービス関数割り込みを呼び出す？？
// いまいち流れがわかってないかも（要復習，今日できれば）

// ↓ それぞれのヘッダファイル内の要素を復習
/*
	- NULL
	- SERIAL_DEFAULT_DEVICE
	- uint8
	- uint16
	- uint32
	- kz_thread_id_t
	- kz_func_t()関数ポインタ
	- kz_handler_t()関数ポインタ
	- kz_msgbox_id_t
*/
#include "defines.h"
#include "kozos.h"
/*
	- SOFTVEC_TYPE_NUM
	- SOFTVEC_TYPE_SOFTERR
	- SOFTVEC_TYPE_SYSCALL
	- SOFTVEC_TYPE_SERINTR
*/
#include "intr.h"
/*
	- SOFTVEC_ADDR
	- softvec_type_t
	- softvec_handler_t()関数ポインタ
	- SOFTVECS
	- INTR_ENABLE
	- INTR_DISABLE
	- softvec_init()
	- softvec_setintr()
	- interrupt()
*/
#include "interrupt.h"
#include "serial.h"
#include "lib.h"
/*
	- CONSDRV_DEVICE_NUM
	- CONSDRV_CMD_USE
	- CONSDRV_CMD_WRITE
*/
#include "consdrv.h"

#define CONS_BUFFER_SIZE 24

static struct consreg
{
	// コンソールを利用したいスレッド
	kz_thread_id_t id;
	// 利用するシリアル番号
	int index;

	// 送信バッファ
	char *send_buf;
	// 受信バッファ
	char *recv_buf;
	// 送信バッファ中のデータサイズ
	int send_len;
	// 受信バッファ中のデータサイズ
	int recv_len;

	// ダミーメンバでサイズ調整（乗算が発生しないように）
	long dummy[3];
} consreg[CONSDRV_DEVICE_NUM];
// 複数のコンソールを管理可能にするために，配列にする

// シリアル送信部分 ----------------------------

// 送信バッファ（変数: send_buf）の先頭一文字を送信
static void send_char(struct consreg *cons)
{
	int i;
	serial_send_byte(cons->index, cons->send_buf[0]);
	cons->send_len--;
	for (i = 0; i < cons->send_len; i++)
		cons->send_buf[i] = cons->send_buf[i + 1];
}

// ①文字列を送信バッファに書き込み
// ②送信割込みを有効
// ③send_char関数を呼び出して先頭一文字を送信
static void send_string(struct consreg *cons, char *str, int len)
{
	// ①
	int i;
	for (i = 0; i < len; i++)
	{
		if (str[i] == '\n')
			cons->send_buf[cons->send_len++] = '\r';
		cons->send_buf[cons->send_len++] = str[i];
	}

	if (cons->send_len && !serial_intr_is_send_enable(cons->index))
	{
		// ②
		serial_intr_send_enable(cons->index);
		// ③
		send_char(cons);
		// 後続の文字列は，送信完了後，送信割り込みの延長で処理される．
	}
}

/*
	以下は割込みハンドラから呼ばれる割込み処理
	つまり、非同期で呼ばれるからライブラリ関数を使うときは注意
	基本的に以下のいずれかに当てはまる関数しか呼び出してはいけない
	・再入可能
	・スレッドから呼ばれることはない関数
	・スレッドから呼ばれることがあるが、割込み禁止で呼び出している
	また非コンテキスト状態で呼ばれるため、システムコールは利用してはいけない
	（代わりにサービスコールを利用すること）
*/
static int consdrv_intrproc(struct consreg *cons)
{
	unsigned char c;
	char *p;

	// 受信割込みの処理
	if (serial_is_recv_enable(cons->index))
	{
		// 受け取る
		c = serial_recv_byte(cons->index);
		// 改行コード変換
		if (c == '\r')
			c = '\n';

		// エコーバック処理
		send_string(cons, &c, 1);

		if (cons->id)
		{
			if (c != '\n')
			{
				// 改行でないなら受信バッファにバッファリング（保存）
				cons->recv_buf[cons->recv_len++] = c;
			}
			else
			{
				// Enterが押されたら、バッファの内容をコマンド処理スレッドに通知する（割込みハンドラ上での処理なので、サービスコールを利用する）
				// 必要なメモリを獲得
				p = kx_kmalloc(CONS_BUFFER_SIZE);
				// 獲得したメモリに受信バッファの内容をコピー
				memcpy(p, cons->recv_buf, cons->recv_len);
				// 割込みハンドラ->スレッド　メッセージを送信
				// MSGBOX_ID_CONSINPUTのメッセージボックスに送信（割込みの延長で処理が行われる）
				kx_send(MSGBOX_ID_CONSINPUT, cons->recv_len, p);
				cons->recv_len = 0;
			}
		}
	}

	// 送信割込みの処理
	if (serial_is_send_enable(cons->index))
	{
		if (!cons->id || !cons->send_len)
		{
			serial_intr_send_disable(cons->index);
		}
		else
		{
			// 送信割込みが発生している場合は、送信バッファに蓄えられているデータの先頭一文字をsend_char()によって出力
			// 文字列の送信は、まず、send_string()によって最初の一文字が送信され、残りは送信完了割込みが発生するたびに、この割込みハンドラから送信される
			// ↓ 先頭の一文字以降の残りの文字列を順番に送信している
			send_char(cons);
			// puts()による送信では送信の完了をビジーループによって待つため無駄があったが、割り込みをベースにした実装にすることで、前の文字の送信が終わるのを割り込みによって待つため、他の処理が動作することができる
			// 他の処理がないならアイドルスレッドが動作して省電力モードに入ることができる
			// CPU時間を無駄にしない
		}

	}
	// ここで注意なのが、send_string()とconsdrv_intrproc()の二箇所からsend_char()が呼ばれ両方から送信バッファが操作されていること
	// send_string()はコンソールドライバスレッドが呼び出すが、consdrv_intrproc()は割り込みハンドラから呼ばれる

	// じゃあスレッドがコンソール主流直の要求を受けて、send_string()を呼び出し、send_char()が呼ばれている最中に以前に送信していた文字の送信が完了して送信割込みが入り
	// 後続の文字を出力するためにsend_char()が呼ばれてしまった時を考えてみよう。

	// この場合、send_char()に対して、11thの「関数の再入と排他』で説明した『関数の再入』が行われてしまう
	// send_char()の内部では送信バッファを操作している。しかし送信バッファの操作中に送信割込みによってsend_char()が再入された場合、さらに送信バッファが上書きされてしまい、誤動作する可能性がある
	// このため送信バッファに対して『排他』の考慮をする必要がある。

	// 『排他』には11thで説明したさまざまな方法があるが、ここでは最も簡単な割込み禁止で回避
	/*
	・割込み禁止
		・OS内部で操作（割込み禁止）
	・セマフォ
	・そもそも再入が入らない設計
	・スレッド化
	*/

	return 0;
}

// シリアルの割込みハンドラ
static void consdrv_intr(void)
{
	int i;
	struct consreg *cons;

	for (i = 0; i < CONSDRV_DEVICE_NUM; i++)
	{
		cons = &consreg[i];
		if (cons->id)
		{
			// シリアルの送信・受信割込みの状態をチェック
			if (serial_is_send_enable(cons->index) || serial_is_recv_enable(cons->index))
				// 割り込みが発生しているのならconsdrv_intrprocを呼び出す
				consdrv_intrproc(cons);
		}
	}
}
