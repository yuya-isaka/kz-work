#include "defines.h"
#include "intr.h"
#include "interrupt.h"

// 割込みベクタは，ハンドラ関数へのポインタ（ベクタアドレス）を持つ配列
// 配列の各インデックスが割込み番号に該当する

// どこから？
// 『bootload/main.c』の『init関数』
// ソフトウェア割込みベクタの初期化
int softvec_init(void)
{
	int sof_type;
	for (sof_type = 0; sof_type < SOFTVEC_TYPE_NUM; sof_type++) // すべてのソフトウェア割込みベクタアドレスをNULLに設定
	{
		// 0 ソフトウェアエラー
		// 1 システムコール
		// 2 シリアル割込み
		softvec_setintr(sof_type, NULL);
	}
	return 0;
}

// どこから？
// 『interrupt.c』の『softvec_init関数』，『kozos.c』の『setintr関数』
// ソフトウェア割込みベクタの設定（ハンドラ関数を登録）
// typeは場所
int softvec_setintr(softvec_type_t sof_type, softvec_handler_t handler)
{
	// 0 ソフトウェアエラー
	// 1 システムコール
	// 2 シリアル割込み
	SOFTVECS[sof_type] = handler;
	return 0;
}

// どこから？
// 『bootload/intr.S』の『_intr_softerr関数』，『_intr_syscall関数』，『_intr_serintr関数』
// 共通割込みハンドラ
// intr.Sから呼ばれていた関数はこれ
// ソフトウェア割込みベクタのアドレスを見て，各ハンドラに分岐する
void interrupt(softvec_type_t type, unsigned long sp)
{
	// 現状，SOFTVECS配列には『thread_intr関数』しか入っていない
	// から必ず『thread_intr関数』が実行される
	softvec_handler_t handler = SOFTVECS[type];
	// ソフトウェア割込みベクタが設定されているならば，ハンドラを呼び出す
	if (handler)
		handler(type, sp);
}
