// シリアルコントローラ用のデバイスドライバの本体

/* レジスタのアクセス方法

	・　分かっているのはコントーラ制御用のレジスタの「アドレス」
	↓
	・　使うためにはポインタ自体にそのアドレスをキャストして無理やり入れて，　参照して読み出したり書き出したりする必要がある
	・　基本的には，　操作用の構造体を用意して，　その構造体ポインタ自体に知っているアドレスをキャストして無理やり入れて操る

	volatile
	・　レジスタに値を書き込む順番が決まってたりするから，　勝手に最適化されると困る
*/

/*
	多くのコントローラは，
		・モード設定
		・制御
		・状態取得
	の３つのレジスタがあり，大体同じようなパターンで使用する．

	例えば，
	初期化を行うには，
		1. コントロールレジスタで動作を停止
		2. モードレジスタを設定
		3. ステータスレジスタで動作を開始
	というのが大まかな手順（プロトコル）

	イベント発生の補足は，
		1. ステータスレジスタを見て何が起きたのか確認(制御を開始していいか確認)
		2. その後でステータスレジスタの当該ビットを落としておく(制御開始の合図)
	という手順（プロトコル)
*/

#include "defines.h"
#include "serial.h"

// ハードウェア制御などに利用される目的の"周辺コントローラなどを内蔵しているCPU"を「マイクロコントローラ」と呼ぶ
// SCIの数 (SCI=シリアルコントローラ)
#define SERIAL_SCI_NUM 3

// SCIの定義
// 先頭アドレス
// 構造体でキャスト，　0xffffb0というアドレスをh8_3069f_sciという構造体でキャスト -> メモリマップドIOで周辺コントローラのアドレス経由でアクセスするとき，　アドレスを構造体に無理やりキャストして入れて，操る
#define H8_3069F_SCI0 ((volatile struct h8_3069f_sci *)0xffffb0)
#define H8_3069F_SCI1 ((volatile struct h8_3069f_sci *)0xffffb8) // シリアルコネクタは1
#define H8_3069F_SCI2 ((volatile struct h8_3069f_sci *)0xffffc0)

// SCIの各種レジスタの定義
// SCIはCPUに内蔵されているのでマッピングされているアドレスは決まっている
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
// シリアルモードレジスタ
// OR論理で結合して設定する
// 現在は，データ長が８ビット，ストップビット長は１ビット，パリティ無しという設定がシリアル接続の標準
// SMRは0で問題ない
#define H8_3069F_SCI_SMR_CKS_PER1 (0 << 0)	// クロックセット（クロックをそのまま利用）
#define H8_3069F_SCI_SMR_CKS_PER4 (1 << 0)	// クロックセット
#define H8_3069F_SCI_SMR_CKS_PER16 (2 << 0) // クロックセット
#define H8_3069F_SCI_SMR_CKS_PER64 (3 << 0) // クロックセット
#define H8_3069F_SCI_SMR_MP (1 << 2)		//
#define H8_3069F_SCI_SMR_STOP (1 << 3)		// ストップビット長が１か2
#define H8_3069F_SCI_SMR_OE (1 << 4)		// 偶数パリティか奇数パリティか
#define H8_3069F_SCI_SMR_PE (1 << 5)		// パリティ無効か有効
#define H8_3069F_SCI_SMR_CHR (1 << 6)		// データ長が8か7
#define H8_3069F_SCI_SMR_CA (1 << 7)		// 調歩動機式モードかクロック同期式モード

// SCRの各ビットの定義
// シリアルコントロールレジスタ
// シリアル入出力の制御(シリアル送受信と割り込み)
// OR論理で結合して設定する
#define H8_3069F_SCI_SCR_CKE0 (1 << 0) // クロックイネーブル
#define H8_3069F_SCI_SCR_CKE1 (1 << 1) // クロックイネーブル
#define H8_3069F_SCI_SCR_TEIE (1 << 2) // クロックイネーブル
#define H8_3069F_SCI_SCR_MPIE (1 << 3) // クロックイネーブル
#define H8_3069F_SCI_SCR_RE (1 << 4)   // 受信有効
#define H8_3069F_SCI_SCR_TE (1 << 5)   // 送信有効
#define H8_3069F_SCI_SCR_RIE (1 << 6)  // 受信割り込み有効
#define H8_3069F_SCI_SCR_TIE (1 << 7)  // 送信割り込み有効

// SSRの各ビットの定義
// シリアルステータスレジスタ
#define H8_3069F_SCI_SSR_MPBT (1 << 0)
#define H8_3069F_SCI_SSR_MPB (1 << 1)
#define H8_3069F_SCI_SSR_TEND (1 << 2)
#define H8_3069F_SCI_SSR_PER (1 << 3)
#define H8_3069F_SCI_SSR_FERERS (1 << 4)
#define H8_3069F_SCI_SSR_ORER (1 << 5)
#define H8_3069F_SCI_SSR_RDRF (1 << 6) // 受信完了
#define H8_3069F_SCI_SSR_TDRE (1 << 7) // 送信完了

// -> reg[].sci でアクセスできるのを作成
// -> その配列の中に「構造体でキャストしたアドレス」を格納
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

// ------------------------------------------------------------------初期化関連-----------------------------------------------------------------------------------

// デバイス初期化
int serial_init(int index)
{
	volatile struct h8_3069f_sci *sci = regs[index].sci;

	// 1. シリアル送受信と割り込みを全て無効化
	sci->scr = 0;
	// 2. コントローラを無効化した後，コントローラのモードを設定
	sci->smr = 0;
	sci->brr = 64; // 20MHzのクロックから9600bpsを生成（水晶振動子を入れ替え25MHzにした場合は80） 1秒間に９６００ビットを転送できる速度, ドキュメントに設定値が書いてある，　SCIのボーレートはSCIに供給されるクロックによて決まる（クロック周波数，分周比）
	// 3. シリアル送受信と割り込みを設定
	sci->scr = H8_3069F_SCI_SCR_RE | H8_3069F_SCI_SCR_TE; // 受信有効，　送信有効
	sci->ssr = 0;

	return 0;
}

// ------------------------------------------------------------------送信関連-----------------------------------------------------------------------------------

// 送信していい?
int serial_is_send_enable(int index)
{
	volatile struct h8_3069f_sci *sci = regs[index].sci;
	// SSRの送信完了ビットを監視
	// 受信側(コントローラ)が準備できたらたつ
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

	// 1.受信側が準備できるま待つ(ビジーループ)
	// 受信準備OK?
	while (!serial_is_send_enable(index))
		;
	/*
		このタイミングで他のスレッドの割込みが入ると？
		Aの処理中断
		Bの処理開始
		Bの処理終わり（送信途中）
		Aの処理再開

		ここで再開してしまうと、すでにビジーループを過ぎているため
		Bの送信途中に送信用レジスタを書き換えるかもしれない。。。。
		これは未定義動作
	*/
	// 2. 書き込みたい文字を書き込む
	sci->tdr = c;
	// 3. SSRの送信完了ビットを落とす(落とすことで送信開始を依頼, 0111111のようなマスクを生成して&=すると7ビット目だけ0にできる)
	sci->ssr &= ~H8_3069F_SCI_SSR_TDRE; // 送信開始
	// 4. 送信完了すると，自動的に送信完了ビット(sci->ssr[7]が立てられる（H８マイコンのシリアルコントローラの設定） ... 自分で作るときもこういう設計にしてドキュメントにまとめる必要があるな

	return 0;
}

// ------------------------------------------------------------------受信関連-----------------------------------------------------------------------------------

// 送信された？受信できる？
int serial_is_recv_enable(int index)
{
	volatile struct h8_3069f_sci *sci = regs[index].sci;

	// sci->ssrレジスタの持つ受信完了ビットを監視
	// (コントローラから)送信されたらビットがたつ
	return (sci->ssr & H8_3069F_SCI_SSR_RDRF);
}

// 1文字受信
unsigned char serial_recv_byte(int index)
{
	volatile struct h8_3069f_sci *sci = regs[index].sci;
	unsigned char c;

	// 送信された？
	// ビジーループ
	while (!serial_is_recv_enable(index))
		;
	// rdrレジスタに受信データ入ってる
	c = sci->rdr;
	// ビットを元に戻す（ビットを落とす？）
	sci->ssr &= ~H8_3069F_SCI_SSR_RDRF;

	return c;
}

// ------------------------------------------------------------------送信割込み関連-----------------------------------------------------------------------------------

// 送信割込み有効？
int serial_intr_is_send_enable(int index)
{
	volatile struct h8_3069f_sci *sci = regs[index].sci;

	return (sci->scr & H8_3069F_SCI_SCR_TIE ? 1 : 0); // SCRのTIEビットを返す
}

// 送信割込み有効化
void serial_intr_send_enable(int index)
{
	volatile struct h8_3069f_sci *sci = regs[index].sci;

	sci->scr |= H8_3069F_SCI_SCR_TIE; // SCRのTIEビットを立てる
}

// 送信割込み無効化
void serial_intr_send_disable(int index)
{
	volatile struct h8_3069f_sci *sci = regs[index].sci;

	sci->scr &= ~H8_3069F_SCI_SCR_TIE; // SCRのTIEビットを落とす
}

// ------------------------------------------------------------------受信割込み関連-----------------------------------------------------------------------------------

// 受信割込み有効？
int serial_intr_is_recv_enable(int index)
{
	volatile struct h8_3069f_sci *sci = regs[index].sci;

	return (sci->scr & H8_3069F_SCI_SCR_RIE ? 1 : 0); // SCRのRIEビットを返す
}

// 受信割込み有効化
void serial_intr_recv_enable(int index)
{
	volatile struct h8_3069f_sci *sci = regs[index].sci;

	sci->scr |= H8_3069F_SCI_SCR_RIE; // SCRのRIEビットを立てる
}

// 受信割込み無効化
void serial_intr_recv_disable(int index)
{
	volatile struct h8_3069f_sci *sci = regs[index].sci;

	sci->scr &= ~H8_3069F_SCI_SCR_RIE; // SCRのRIEビットを落とす
}
