# スタートアップ
# 拡張子が小文字のSだたら，　プリプロセスが行われない

	.h8300h
	.section .text

	.global _start
	.type _start, @function

# どこから？
# CPUの電源ON割り込み，リセット割り込み　<- 『リセットベクタ』
_start: # ラベル_startの定義
	mov.l #_bootstack,sp # スタックポインタの設定, ベタガキではなくシンボル参照
	jsr @_main # main()の呼び出し

1:
	bra 1b
