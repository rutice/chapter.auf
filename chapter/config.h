//---------------------------------------------------------------------
//		プラグイン設定ヘッダファイル
//---------------------------------------------------------------------
#include <windows.h>
#include <stdio.h>
#include "filter.h"

#define NUMHIS 30	// 保存する履歴の数
#define STRLEN 256	// 文字列の最大長

class CfgDlg;

typedef struct 
{
	// チャプタ数は書式上最大100個まで
	int m_numChapter;
	int m_Frame[100];
	char m_strTitle[100][STRLEN];
} PrfDat;

class CfgDlg
{
	HWND m_hDlg;
	FILTER *m_fp;
	EXFUNC *m_exfunc;
	void *m_editp;
	int m_scale;
	int m_rate;
	int m_frame;
	int m_numFrame;
	char m_strTime[STRLEN];
	char m_strHis[NUMHIS][STRLEN];
	int m_numHis;
	bool m_loadfile;
	int m_autosave;

	void AddHis();

public:
	int m_numChapter;
	int m_Frame[100];
	char m_strTitle[100][STRLEN];

	void ShowList();
	void Init(HWND hwnd,void *editp,FILTER *fp);
	void SetFrame(int frame);
	void SetFps(int rate,int scale);
	void SetFrameN(void *editp,int frame_n);
	void AddList();
	void DelList();
	void Seek();
	void Save();
	void AutoSave();
	void Load();
	void AuotSaveCheck();
};
