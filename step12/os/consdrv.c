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

static int consdrv_intrproc(struct consreg *cons)
{
	unsigned char c;
	char *p;

	if (serial_is_recv_enable(cons->index))
	{
		// 受け取る
		c = serial_recv_byte(cons->index);
		if (c == '\r')
			c = '\n';

		// エコーバック処理
		send_string(cons, &c, 1);

		if (cons->id)
		{
			if (c != '\n')
			{
				// 受信バッファに保存
				cons->recv_buf[cons->recv_len++] = c;
			}
			else
			{
				p = kx_kmalloc(CONS_BUFFER_SIZE);
				memcpy(p, cons->recv_buf, cons->recv_len);
				kx_send(MSGBOX_ID_CONSINPUT, cons->recv_len, p);
				cons->recv_len = 0;
			}
		}
	}

	if (serial_is_send_enable(cons->index))
	{
		if (!cons->id || !cons->send_len)
		{
			serial_intr_send_disable(cons->index);
		}
		else
		{
			send_char(cons);
		}
	}

	return 0;
}

static void consdrv_intr(void)
{
	int i;
	struct consreg *cons;

	for (i = 0; i < CONSDRV_DEVICE_NUM; i++)
	{
		cons = &consreg[i];
		if (cons->id)
		{
			if (serial_is_send_enable(cons->index) || serial_is_recv_enable(cons->index))
				consdrv_intrproc(cons);
		}
	}
}


