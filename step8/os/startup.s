# スタートアップ
	.h8300h
	.section .text

	.global _start
	.type 	_start, @function

_start: # ラベル_startの定義
	# スタックポインタの設定, ベタガキではなくシンボル参照
	mov.l #_bootstack, sp
	# main()の呼び出し
	jsr @_main

1:
	bra 1b

	.global _dispatch
	.type 	_dispatch, @function

# どこから？
# 『kozos.c』の『kz_start関数』と『thread_intr関数』
# スレッドのディスパッチ処理（ディスパッチャ）
_dispatch:
	# スタックポインタを第一引数(er0）として渡された『スレッドのスタックポインタ』で上書き
	# 以降はer7はスレッドのスタック領域を指すようになる
	mov.l	@er0, er7
	# スレッドのスタックから，汎用レジスタの値を復旧する
	# 復旧する情報は， 『thread_run関数』で新規作成する時に，　事前にスタックに格納している（0埋め）
	# -> 直接0を埋めてもいいように見えるが，　割込み発生時の処理と共通化するために，汎用レジスタの退避と復旧を必ず実行する
	mov.l	@er7+, er0 # ロードしたら4バイト加算
	mov.l	@er7+, er1
	mov.l	@er7+, er2
	mov.l	@er7+, er3
	mov.l	@er7+, er4
	mov.l	@er7+, er5
	mov.l	@er7+, er6
	# 割込み復帰命令で， CPUによってPCとCCRが変更され（コンテキスト切り替え），　スタック領域の一番上に積まれている『thread_init関数』が実行される
	rte