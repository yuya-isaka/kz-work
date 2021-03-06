#include "defines.h"
#include "serial.h"

#define SERIAL_SCI_NUM 3 // SCIの数

// SCIの定義
// 既に構造体の中でvolatileしているから、ここのvolatileはいらない
// 本当かな？　実験してみる価値はあり
#define H8_3069F_SCI0 ((volatile struct h8_3069f_sci *)0xffffb0)
#define H8_3069F_SCI1 ((volatile struct h8_3069f_sci *)0xffffb8)
#define H8_3069F_SCI2 ((volatile struct h8_3069f_sci *)0xffffc0)

// SCIの各種レジスタの定義
struct h8_3069f_sci
{
	volatile uint8 smr; // 0xffffb8, シリアル通信のモード設定
	volatile uint8 brr; // 0xffffb9, シリアル通信の速度（ボーレート)の設定, ビットレートレジスタ
	volatile uint8 scr; // 0xffffba, 送受信の有効/無効
	volatile uint8 tdr; // 0xffffbb, 送信したい1文字を書き込む, トランスミットデータレジスタ
	volatile uint8 ssr; // 0xffffbc, 送信完了/受信完了などを表す
	volatile uint8 rdr; // 0xffffbd, 受信した１文字を読み出す
	volatile uint8 scmr;
};

// SMRの各ビットの定義
#define H8_3069F_SCI_SMR_CKS_PER1 (0 << 0)	// 00000000 クロックセット（クロックをそのまま利用）
#define H8_3069F_SCI_SMR_CKS_PER4 (1 << 0)	// 00000001 クロックセット
#define H8_3069F_SCI_SMR_CKS_PER16 (2 << 0) // 00000010 クロックセット
#define H8_3069F_SCI_SMR_CKS_PER64 (3 << 0) // 00000011 クロックセット
#define H8_3069F_SCI_SMR_MP (1 << 2)		// 00000100
#define H8_3069F_SCI_SMR_STOP (1 << 3)		// 00001000 ストップビット長が１か2
#define H8_3069F_SCI_SMR_OE (1 << 4)		// 00010000 偶数パリティか奇数パリティか
#define H8_3069F_SCI_SMR_PE (1 << 5)		// 00100000 パリティ無効か有効
#define H8_3069F_SCI_SMR_CHR (1 << 6)		// 01000000 データ長が8か7
#define H8_3069F_SCI_SMR_CA (1 << 7)		// 10000000 調歩動機式モードかクロック同期式モード

// SCRの各ビットの定義
#define H8_3069F_SCI_SCR_CKE0 (1 << 0) // クロックイネーブル
#define H8_3069F_SCI_SCR_CKE1 (1 << 1) // クロックイネーブル
#define H8_3069F_SCI_SCR_TEIE (1 << 2) // クロックイネーブル
#define H8_3069F_SCI_SCR_MPIE (1 << 3) // クロックイネーブル
#define H8_3069F_SCI_SCR_RE (1 << 4)   // 受信有効
#define H8_3069F_SCI_SCR_TE (1 << 5)   // 送信有効
#define H8_3069F_SCI_SCR_RIE (1 << 6)  // 受信割り込み有効
#define H8_3069F_SCI_SCR_TIE (1 << 7)  // 送信割り込み有効

// SSRの各ビットの定義
#define H8_3069F_SCI_SSR_MPBT (1 << 0)
#define H8_3069F_SCI_SSR_MPB (1 << 1)
#define H8_3069F_SCI_SSR_TEND (1 << 2)
#define H8_3069F_SCI_SSR_PER (1 << 3)
#define H8_3069F_SCI_SSR_FERERS (1 << 4)
#define H8_3069F_SCI_SSR_ORER (1 << 5)
#define H8_3069F_SCI_SSR_RDRF (1 << 6) // 受信完了
#define H8_3069F_SCI_SSR_TDRE (1 << 7) // 送信完了

static struct
{
	volatile struct h8_3069f_sci *sci;
	/*
		SMR
		BRR
		SCR
		TDR
		SSR
		RDR
		SCMR
	*/
} regs[SERIAL_SCI_NUM] = {
	{H8_3069F_SCI0}, // <- h8_3069f_sci構造体(sciと同じ構造)だったら入る
	{H8_3069F_SCI1}, // SCI1の先頭アドレス
	{H8_3069F_SCI2}, // SCI2の先頭アドレス
};

// デバイス初期化
int serial_init(int index)
{
	volatile struct h8_3069f_sci *sci = regs[index].sci;

	// 1. シリアル送受信と割り込みを全て無効化
	sci->scr = 0;
	// 2. コントローラを無効化した後，コントローラのモードを設定
	sci->smr = 0;
	sci->brr = 64; // 20MHzのクロックから9600bpsを生成（水晶発信器を入れ替え25MHzにした場合は80） 1秒間に９６００ビットを転送できる速度, ドキュメントに設定値が書いてある，　SCIのボーレートはSCIに供給されるクロックによて決まる（クロック周波数，分周比）
	// 3. シリアル送受信と割り込みを設定
	sci->scr = H8_3069F_SCI_SCR_RE | H8_3069F_SCI_SCR_TE; // 受信有効，　送信有効
	sci->ssr = 0;

	return 0;
}

// 送信可能か？
int serial_is_send_enable(int index)
{
	volatile struct h8_3069f_sci *sci = regs[index].sci;
	// SSRの送信完了ビットを監視
	return (sci->ssr & H8_3069F_SCI_SSR_TDRE);
}

// 1文字送信
/* 1文字送信の手順
	1. SSRの送信完了ビットが落ちていないことを確認
	2. TDRに送信したい文字を書きこむ
	3. SSRの送信完了ビットを落とす
	4. 送信が完了すると，コントローラがSSRの送信完了ビットを立てる
	(送信が完了するまで次の送信文字をTDRに書き込んではいけない)
*/
// シリアルへの文字出力関数(端末変換を行わない)
int serial_send_byte(int index, unsigned char c)
{
	volatile struct h8_3069f_sci *sci = regs[index].sci;

	// 1. 送信可能になるまで待つ(ビジーループ)
	while (!serial_is_send_enable(index))
		;
	// 2. 書き込みたい文字を書き込む
	sci->tdr = c;
	// 3. SSRの送信完了ビットを落とす(落とすことで送信開始を依頼, 0111111のようなマスクを生成して&=すると7ビット目だけ0にできる)
	sci->ssr &= ~H8_3069F_SCI_SSR_TDRE; // 送信開始
	// 4. 送信完了すると，自動的に送信完了ビット(sci->ssr[7]が立てられる（H８マイコンのシリアルコントローラの設定） ... 自分で作るときもこういう設計にしてドキュメントにまとめる必要があるな

	return 0;
}
