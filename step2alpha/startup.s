	.h8300h
	.section .text
	.global _start
	.type _start, @function

# スタートアップ
_start:
	mov.l #0xffff00,sp # スタックポインタの設定
	jsr @_main # main()の呼び出し

# 無限ループ
1:
	bra 1b
