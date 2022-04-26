// 様々な方の定義
// 頻繁に利用する型は定義しておく（ヘッダファイルの依存関係をなくすため）

#ifndef _DEFINES_H_INCLUDED_
#define _DEFINES_H_INCLUDED_

#define NULL ((void *)0)		// NULLポインタの定義
#define SERIAL_DEFAULT_DEVICE 1 // 標準のシリアルデバイス

// ビット幅固の整数型
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long uint32;

typedef uint32 kz_thread_id_t;
typedef int (*kz_func_t)(int argc, char *argv[]);
typedef void (*kz_handler_t)(void);

// メッセージIDの定義
typedef enum
{
	// コンソールからの入力用
	// シリアル受信割込みの発生時に割り込みハンドラから受信文字をコマンドスレッドに送信する際に、メッセージボックスとして利用される
	MSGBOX_ID_CONSINPUT = 0,
	// コンソールへの出力用
	// コマンドスレッドからコンソールドライバスレッドに文字列の出力依頼を行う際に、メッセージボックスとして利用される
	MSGBOX_ID_CONSOUTPUT,
	// 最後にNUMを持ってくることで，数を表すようにしている．賢い！
	MSGBOX_ID_NUM,
} kz_msgbox_id_t;

#endif
