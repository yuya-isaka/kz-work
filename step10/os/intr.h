// 割り込み種別の定義

#ifndef _INTR_H_INCLUDED_
#define _INTR_H_INCLUDED_

// ソフトウェア・割込みベクタの定義

// ソフトウェア・割込みベクタの種別の個数
// なぜ整数で定義するの？
// -> intr.Sのアセンブラからは，typedefとかenumとかを解釈できないので，整数として定義して使えるようにしている
#define SOFTVEC_TYPE_NUM 3

#define SOFTVEC_TYPE_SOFTERR 0 // ソフトウェアエラー
#define SOFTVEC_TYPE_SYSCALL 1 // システム・コール
#define SOFTVEC_TYPE_SERINTR 2 // シリアル割込み

#endif
