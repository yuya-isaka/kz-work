// シリアルコントローラ用のデバイスドライバの本体

#include "defines.h"
#include "serial.h"

// ハードウェア制御などに利用される目的の"周辺コントローラなどを内蔵しているCPU"を「マイクロコントローラ」と呼ぶ
// SCIの数 (SCI=シリアルコントローラ)
#define SERIAL_SCI_NUM 3

// SCIの定義
// 先頭アドレス
// 構造体でキャスト，　0xffffb0というアドレスをh8_3069f_sciという構造体でキャスト -> メモリマップドIOで周辺コントローラのアドレス経由でアクセスするとき，　アドレスを構造体に無理やりキャストして入れて，操る
#define SCI0 ((volatile sci *)0xffffb0)
#define SCI1 ((volatile sci *)0xffffb8) // シリアルコネクタは1
#define SCI2 ((volatile sci *)0xffffc0)

// SCIの各種レジスタの定義
// SCIはCPUに内蔵されているのでマッピングされているアドレスは決まっている
typedef struct _sci
{
	volatile uint8 smr; // 0xffffb8, シリアル通信のモード設定
	volatile uint8 brr; // 0xffffb9, シリアル通信の速度（ボーレート)の設定, ビットレートレジスタ
	volatile uint8 scr; // 0xffffba, 送受信の有効/無効
	volatile uint8 tdr; // 0xffffbb, 送信したい1文字を書き込む, トランスミットデータレジスタ
	volatile uint8 ssr; // 0xffffbc, 送信完了/受信完了などを表す
	volatile uint8 rdr; // 0xffffbd, 受信した１文字を読み出す
	volatile uint8 scmr;
} sci;

// SMRの各ビットの定義
// シリアルモードレジスタ
// OR論理で結合して設定する
// 現在は，データ長が８ビット，ストップビット長は１ビット，パリティ無しという設定がシリアル接続の標準
// SMRは0で問題ない
#define SMR_CKS_PER1 (0 << 0)  // クロックセット（クロックをそのまま利用）
#define SMR_CKS_PER4 (1 << 0)  // クロックセット
#define SMR_CKS_PER16 (2 << 0) // クロックセット
#define SMR_CKS_PER64 (3 << 0) // クロックセット
#define SMR_MP (1 << 2)		   //
#define SMR_STOP (1 << 3)	   // ストップビット長が１か2
#define SMR_OE (1 << 4)		   // 偶数パリティか奇数パリティか
#define SMR_PE (1 << 5)		   // パリティ無効か有効
#define SMR_CHR (1 << 6)	   // データ長が8か7
#define SMR_CA (1 << 7)		   // 調歩動機式モードかクロック同期式モード

// SCRの各ビットの定義
// シリアルコントロールレジスタ
// シリアル入出力の制御(シリアル送受信と割り込み)
// OR論理で結合して設定する
#define SCR_CKE0 (1 << 0) // クロックイネーブル
#define SCR_CKE1 (1 << 1) // クロックイネーブル
#define SCR_TEIE (1 << 2) // クロックイネーブル
#define SCR_MPIE (1 << 3) // クロックイネーブル
#define SCR_RE (1 << 4)	  // 受信有効
#define SCR_TE (1 << 5)	  // 送信有効
#define SCR_RIE (1 << 6)  // 受信割り込み有効
#define SCR_TIE (1 << 7)  // 送信割り込み有効

// SSRの各ビットの定義
// シリアルステータスレジスタ
#define SSR_MPBT (1 << 0)
#define SSR_MPB (1 << 1)
#define SSR_TEND (1 << 2)
#define SSR_PER (1 << 3)
#define SSR_FERERS (1 << 4)
#define SSR_ORER (1 << 5)
#define SSR_RDRF (1 << 6) // 受信完了
#define SSR_TDRE (1 << 7) // 送信完了

static volatile sci *regs[SERIAL_SCI_NUM] = {
	SCI0,
	SCI1,
	SCI2,
};

// デバイス初期化
int serial_init(int index)
{
	volatile sci *s = regs[index];

	// 1. シリアル送受信と割り込みを全て無効化
	s->scr = 0;
	// 2. コントローラを無効化した後，コントローラのモードを設定
	s->smr = 0;
	s->brr = 64; // ボーレート設定
	// 3. シリアル送受信と割り込みを設定
	s->scr = SCR_RE | SCR_TE; // 受信有効，　送信有効
	s->ssr = 0;

	return 0;
}

// 送信可能か？
int serial_is_send_enable(int index)
{
	volatile sci *s = regs[index];
	// SSRの送信完了ビットを監視
	return (s->ssr & SSR_TDRE);
}

// 1文字送信
int serial_send_byte(int index, unsigned char c)
{
	volatile sci *s = regs[index];

	// 1. 送信可能になるまで待つ(ビジーループ)
	while (!serial_is_send_enable(index))
		;
	// 2. 書き込みたい文字を書き込む
	s->tdr = c;
	// 3. SSRの送信完了ビットを落とす(落とすことで送信開始を依頼, 0111111のようなマスクを生成して&=すると7ビット目だけ0にできる)
	s->ssr &= ~SSR_TDRE; // 送信開始
	// 4. 送信完了すると，自動的に送信完了ビット(sci->ssr[7]が立てられる（H８マイコンのシリアルコントローラの設定） ... 自分で作るときもこういう設計にしてドキュメントにまとめる必要があるな

	return 0;
}
