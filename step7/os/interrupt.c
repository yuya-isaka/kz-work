#include "defines.h"
#include "intr.h"
#include "interrupt.h"

// 割込みベクタは，ハンドラ関数へのポインタ（ベクタアドレス）を持つ配列
// 配列の各インデックスが割込み番号に該当する

// ソフトウェア割込みベクタの初期化
int softvec_init(void)
{
	int type;
	for (type = 0; type < SOFTVEC_TYPE_NUM; type++) // すべてのソフトウェア割込みベクタアドレスをNULLに設定
	{
		// 0 ソフトウェアエラー
		// 1 システムコール
		// 2 シリアル割込み
		softvec_setintr(type, NULL);
	}
	return 0;
}

// ソフトウェア割込みベクタの設定（ハンドラ関数を登録）
// typeは場所
int softvec_setintr(softvec_type_t type, softvec_handler_t handler)
{
	// 0 ソフトウェアエラー
	// 1 システムコール
	// 2 シリアル割込み
	SOFTVECS[type] = handler;
	return 0;
}

// 共通割込みハンドラ
// intr.Sから呼ばれていた関数はこれ
// ソフトウェア割込みベクタのアドレスを見て，各ハンドラに分岐する
void interrupt(softvec_type_t type, unsigned long sp)
{
	softvec_handler_t handler = SOFTVECS[type];
	// ソフトウェア割込みベクタが設定されているならば，ハンドラを呼び出す
	if (handler)
		handler(type, sp);
}
