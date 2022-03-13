// ファイル転送プロトコル

#include "defines.h"
#include "serial.h"
#include "lib.h"
#include "xmodem.h"

/*
	受信側の処理
	① 受信準備ができたら，合図として定期的にNAK(0x15)を送信する．
		シリアルから文字を受信したらデータ受信開始する．
	② SOH(0x01)を受け取ったら， 連続するデータをブロックとして受信
			1. ブロック番号の受信
			2. 反転したブロック番号の受信
			3. 128バイトのデータを受信
			4. チェックサムの受信
		受信に成功したらACK(0x06)を返す
		受信に失敗したらNAK(0x15)を返す　（チェックサムエラー）
	③ EOT(0x04)を受けたら， ACK(0x06)を返して終了する
	④ 中断したい場合はCAN(0x18)を送信する．
		CAN(0x18)を受けたら中断する．
*/

// 制御コードの定義
#define XMODEM_SOH 0x01
#define XMODEM_STX 0x02
#define XMODEM_EOT 0x04
#define XMODEM_ACK 0x06
#define XMODEM_NAK 0x15
#define XMODEM_CAN 0x18
#define XMODEM_EOF 0x1a // ctrl-z

#define XMODEM_BLOCK_SIZE 128

// 受信開始されるまで送信要求出す （受信側①）
static int xmodem_wait(void)
{
	long cnt = 0;

	// 受信開始するまで，　NAKを定期的に送信する
	while (!serial_is_recv_enable(SERIAL_DEFAULT_DEVICE))
	{
		// バグになり得る．　serial_is_recv_enableでチェックが行われた直後で，カウンタが条件を満たして，NAKを送信してしまう可能性がある　-> 受信エラーになる
		// まあ失敗したらリセットしてやり直せば良い
		if (++cnt >= 2000000) // 8秒間隔
		{
			cnt = 0;
			serial_send_byte(SERIAL_DEFAULT_DEVICE, XMODEM_NAK);
		}
	}

	return 0;
}

// ブロック単位の受信 (受信側②)
static int xmodem_read_block(unsigned char block_number, char *buf)
{
	unsigned char c, block_num, check_sum;
	int i;

	// 1. ブロック番号の受信
	block_num = serial_recv_byte(SERIAL_DEFAULT_DEVICE);
	if (block_num != block_number)
		return -1; // 想定しているブロック番号じゃなかたらエラー

	// 2. 反転したブロック番号の受信
	block_num ^= serial_recv_byte(SERIAL_DEFAULT_DEVICE);
	if (block_num != 0xff)
		return -1; // 想定しているブロック番号じゃなかたらエラー

	// 3. 128バイトのデータを受信
	check_sum = 0;
	for (i = 0; i < XMODEM_BLOCK_SIZE; i++)
	{
		// 1文字の受信
		c = serial_recv_byte(SERIAL_DEFAULT_DEVICE);
		*(buf++) = c;
		check_sum += c;
	}

	// 4. チェックサムの受信
	check_sum ^= serial_recv_byte(SERIAL_DEFAULT_DEVICE);
	if (check_sum)
		return -1; // 想定しているブロック番号じゃなかたらエラー

	return i;
}

// XMODEMの全体処理
// すべての受信したブロックのサイズを返す
long xmodem_recv(char *buf)
{
	int r, receiving = 0;
	long size = 0;
	unsigned char c, block_number = 1;

	while (1)
	{
		// 一度送信されたら値が続いてくる
		if (!receiving)
			xmodem_wait(); // 受信開始されるまで送信要求を出す

		// 1文字の受信
		c = serial_recv_byte(SERIAL_DEFAULT_DEVICE);

		if (c == XMODEM_EOT) // EOTを受信したら終了 (受信側③)
		{
			serial_send_byte(SERIAL_DEFAULT_DEVICE, XMODEM_ACK);
			break;
		}
		else if (c == XMODEM_CAN) // CANを受信したら中断 (受信側④)
		{
			return -1;
		}
		else if (c == XMODEM_SOH) // SOHを受信したらデータ受信を開始 (受信側②)
		{
			receiving++;
			r = xmodem_read_block(block_number, buf); // ブロック単位での受信

			if (r < 0) // 受信エラー時にはNAK返す
			{
				serial_send_byte(SERIAL_DEFAULT_DEVICE, XMODEM_NAK);
			}
			else // 正常受信時にはバッファのポインタを進めACKを返す
			{
				block_number++;
				size += r;
				buf += r;
				serial_send_byte(SERIAL_DEFAULT_DEVICE, XMODEM_ACK);
			}
		}
		else
		{
			// 一度送信された後は値がくるはず，来なかったらエラー
			if (receiving)
				return -1;
		}
	}

	return size;
}