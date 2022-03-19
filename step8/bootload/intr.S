# 割込みハンドラの実装

# 拡張子が大文字のSだたら，　プリプロセスが行われる
# -> 内部で「SOFTVEC_TYPE_SOFTERR」などのキーワードがあるので，　intr.hのインクルードと#defineのキーワード変換が必要

# 3種類の関数を定義
# アセンブラの利用を最小限にするために，　ここに書くのは割込みハンドラの「入口と出口」のみ

#include "intr.h"
	.h8300h
	.section .text

# ソフトウェアエラー割込みハンドラ

	# _intr_softerr関数を外部に公開し他のファイルから参照できるようにしている
	.global _intr_softerr
	.type _intr_softerr, @function

_intr_softerr:
	# 入口-------------------------------------------
	# 汎用レジスタの値をスタックに保存 (プログラム・カウンタとCCRの値は，割り込み発生時にCPUが自動的にスタックにアトミックに保存するが，　汎用レジスタはしない，　ので手動でやる必要がある)
	# 本来は，　汎用レジスタの他にも利用しているレジスタがあれば保存してやる必要がある．今回は，　フラグレジスタ(オーバーフローフラグ，キャリフラグ)などは，CPUが自動的に退避させているから考慮しなくて良い
	mov.l	er6, @-er7
	mov.l	er5, @-er7
	mov.l	er4, @-er7
	mov.l	er3, @-er7
	mov.l	er2, @-er7
	mov.l	er1, @-er7
	mov.l	er0, @-er7
	# ------------------------------------------------

	# 第2引数にスタックポインタを設定
	# -> 第２引数を見れば保存されている汎用レジスタの値を知ることができる．
	# -> ポインタ値を渡すから３２ビット整数 (H8/3069Fのアドレス範囲は24ビット（16Mバイト）)
	# -> -mint32オプションをつけてコンパイルしたら，　1ワードを32ビットと扱い， int型にERxという32ビットレジスタが割り当てられる (本書では利用しない)
	# -> 一応上のオプションを考慮して， interrupt.hでは　割込みの種別はshort型として定義してある
	mov.l	er7, er1

	# 割り込みスタックを利用
	mov.l	#_intrstack, sp
	# 旧スタックポインタの値を現在のスタック　（割り込みスタック）　に保存する
	mov.l	er1, @-er7

	# 第１引数にintr.hの「SOFTVEC_TYPE_SOFTERR」を設定
	# -> 第１引数を見れば割込みの種類がわかる
	# -> R0で16ビットレジスタで渡している int型 (int型は各CPUのワードサイズ)
	mov.w	#SOFTVEC_TYPE_SOFTERR, r0

	# 関数interrupt()の呼び出し
	# 実際の割り込みハンドラは「C言語」で書く
	jsr		@_interrupt

	# 旧スタックポインタの値を割込みスタックから普及する
	mov.l	@er7+, er1
	mov.l	er1, er7

	# 出口---------------------------------------------
	# スタックから汎用レジスタの値を復旧する
	mov.l	@er7+, er0
	mov.l	@er7+, er1
	mov.l	@er7+, er2
	mov.l	@er7+, er3
	mov.l	@er7+, er4
	mov.l	@er7+, er5
	mov.l	@er7+, er6

	# 割込み復帰命令の実行
	rte
	# -----------------------------------------------


# システムコール割込みハンドラ--------------------------------------------

	.global	_intr_syscall
	.type	_intr_syscall, @function

_intr_syscall:
	mov.l	er6, @-er7
	mov.l	er5, @-er7
	mov.l	er4, @-er7
	mov.l	er3, @-er7
	mov.l	er2, @-er7
	mov.l	er1, @-er7
	mov.l	er0, @-er7

	mov.l	er7, er1
	mov.w	#SOFTVEC_TYPE_SYSCALL, r0
	jsr		@_interrupt

	mov.l	@er7+, er0
	mov.l	@er7+, er1
	mov.l	@er7+, er2
	mov.l	@er7+, er3
	mov.l	@er7+, er4
	mov.l	@er7+, er5
	mov.l	@er7+, er6
	rte


# シリアル割込みハンドラ ------------------------------------------ ブートローダー側で準備

	.global	_intr_serintr
	.type	_intr_serintr, @function 	# なくてもいい，　デバッグよう

_intr_serintr:
	mov.l	er6, @-er7
	mov.l	er5, @-er7
	mov.l	er4, @-er7
	mov.l	er3, @-er7
	mov.l	er2, @-er7
	mov.l	er1, @-er7
	mov.l	er0, @-er7

	mov.l	er7, er1
	mov.w	#SOFTVEC_TYPE_SERINTR, r0 # interrupt.cの関数interrupt()
	jsr		@_interrupt

	mov.l	@er7+, er0
	mov.l	@er7+, er1
	mov.l 	@er7+, er2
	mov.l 	@er7+, er3
	mov.l 	@er7+, er4
	mov.l 	@er7+, er5
	mov.l 	@er7+, er6
	rte