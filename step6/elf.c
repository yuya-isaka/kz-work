#include "defines.h"
#include "elf.h"
#include "lib.h"

struct elf_header
{
	// 16バイトの識別情報
	struct
	{
		unsigned char magic[4];	   // マジックナンバ
		unsigned char class;	   // 32 or 64
		unsigned char format;	   // エンディアン
		unsigned char version;	   // ELFフォーマットのバージョン
		unsigned char abi;		   // OS種類
		unsigned char abi_version; // OSバージョン
		unsigned char reserve[7];  // 予約
	} id;
	short type;					// ファイル種別
	short arch;					// CPU種類
	long version;				// ELF形式のバージョン
	long entry_point;			// 実行開始アドレス
	long program_header_offset; // プログラム・ヘッダ・テーブルの位置
	long section_header_offset; // セクション・ヘッダ・テーブルの位置
	long flags;					// 各種フラグ
	short header_size;			// ELFヘッダのサイズ
	short program_header_size;	// プログラム・ヘッダのサイズ
	short program_header_num;	// プログラム・ヘッダの個数
	short section_header_size;	// セクション・ヘッダのサイズ
	short section_header_num;	// セクション・ヘッダの個数
	short section_name_index;	// セクション名を格納するセクション
};

// プログラム・ヘッダの定義
struct elf_program_header
{
	long type;			// セグメントとの種別 (ロード可能か？)
	long offset;		// ファイル中の位置
	long virtual_addr;	// 仮想アドレス
	long physical_addr; // 物理アドレス
	long file_size;		// ファイル中のサイズ
	long memory_size;	// メモリ上でのサイズ
	long flags;			// 各種フラグ
	long align;			// アライメント
};

// ELFヘッダのチェック
static int elf_check(struct elf_header *header)
{
	// マジックナンバのチェック
	if (memcmp(header->id.magic, "\x7f"
								 "ELF",
			   4))
		return -1;

	// ↓ フォーマット解析用の構造体を定義し，構造体のポインタを利用することで，フォーマット内のパラメータを参照できる

	// ELF32
	if (header->id.class != 1)
		return -1;
	// ビッグエンディアン
	if (header->id.format != 2)
		return -1;
	// version 1
	if (header->id.version != 1)
		return -1;
	// 実行形式ファイル
	if (header->type != 2)
		return -1;
	// version 1
	if (header->id.version != 1)
		return -1;

	// H8/300H or H8/300
	if ((header->arch != 46) && (header->arch != 47))
		return -1;

	return 0;
}

// プログラム・ヘッダの解析
// セグメント単位でのロード
static int elf_load_program(struct elf_header *header)
{
	int i;
	struct elf_program_header *phdr;

	// セグメント単位でループ
	// セグメント情報の実体は，プログラム・ヘッダの集まり．
	for (i = 0; i < header->program_header_num; i++)
	{
		// プログラムヘッダを取得 (これも構造体のポインタを利用することで，アクセス)
		//                                           ↓ アドレス
		phdr = (struct elf_program_header *)((char *)header + header->program_header_offset + header->program_header_size * i);

		// ロード可能か？
		if (phdr->type != 1)
			continue;

		// とりあえず実験用に，実際にロードせずにセグメント情報を表示する
		putxval(phdr->offset, 6);
		puts(" ");
		putxval(phdr->virtual_addr, 8);
		puts(" ");
		putxval(phdr->physical_addr, 8);
		puts(" ");
		putxval(phdr->file_size, 5);
		puts(" ");
		putxval(phdr->memory_size, 5);
		puts(" ");
		putxval(phdr->flags, 2);
		puts(" ");
		putxval(phdr->align, 2);
		puts(" ");
	}

	return 0;
}

// ELF形式の解析
int elf_load(char *buf)
{
	// ELF形式を保存したバッファのアドレス
	// アドレスを構造体へのポインタにキャストすることで，構造体を利用してのフォーマット解析が可能に．．．え，なんでバッファのアドレスから構造体にキャストして利用できるの？？？？ -> バッファないで並んでるバイトの順番を考慮して，構造体を定義しているからそこに綺麗に当てはまる？？
	struct elf_header *header = (struct elf_header *)buf;

	// ELFヘッダのチェック
	if (elf_check(header) < 0)
		return -1;

	// セグメント単位でのロード
	if (elf_load_program(header) < 0)
		return -1;

	return 0;
}
