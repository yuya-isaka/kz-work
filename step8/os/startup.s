# スタートアップ
	.h8300h
	.section .text

	.global _start
	.type 	_start, @function

_start: # ラベル_startの定義
	mov.l #_bootstack, sp # スタックポインタの設定, ベタガキではなくシンボル参照
	jsr @_main # main()の呼び出し

1:
	bra 1b

	.global _dispatch
	.type 	_dispatch, @function

# スレッドのディスパッチ処理（ディスパッチャ）
_dispatch:
	mov.l	@er0, er7	# 引数としてスレッドのスタックポインタが渡される
	# スレッドのスタックから汎用レジスタの値を復旧する
	mov.l	@er7+, er0 // ロードしたら4バイト加算
	mov.l	@er7+, er1
	mov.l	@er7+, er2
	mov.l	@er7+, er3
	mov.l	@er7+, er4
	mov.l	@er7+, er5
	mov.l	@er7+, er6
	rte // 割込み復帰命令で，コンテキスト切り替え（PCとCCRの変更, CPUが実行する）