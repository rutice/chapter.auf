//---------------------------------------------------------------------
//		プラグイン設定ヘッダファイル
//---------------------------------------------------------------------
#include <windows.h>
#include <stdio.h>
#include <vector>
#include <string>
#include "filter.h"

#ifndef _CHAPTER_CONFIG_H_
#define _CHAPTER_CONFIG_H_

const int NUMHIS = 50;	// 保存する履歴の数
const int STRLEN = 256;	// 文字列の最大長

const int MAXCHAPTER = 500;

class CfgDlg;

// aupに保存する内容。現状100個まで
typedef struct 
{
	// チャプタ数は書式上最大100個まで
	int m_numChapter;
	int m_Frame[100];
	char m_strTitle[100][STRLEN];
	int m_SCPos[100];
} PrfDat;

typedef struct {
	int frame;
	std::string title;
} chapter;

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
	bool SaveToFile(const char *lpFile);

public:
	int m_numChapter;
	int m_Frame[MAXCHAPTER];
	char m_strTitle[MAXCHAPTER][STRLEN];
	int m_SCPos[MAXCHAPTER];

	void ShowList(int defSelect = -1);
	void Init(HWND hwnd,void *editp,FILTER *fp);
	void Resize();
	void SetFrame(int frame);
	void SetFps(int rate,int scale);
	void SetFrameN(void *editp,int frame_n);
	void AddList();
	void DelList();
	void NextList();
	void PrevList();
	void NextHereList();
	void PrevHereList();
	void Seek();
	void Save();
	void AutoSave();
	void Load();
	void AutoSaveCheck();
	int GetSCPos(int moveto, int frames);

	//[ru]関数追加
	void LoadFromFile(char *filename);
	void DetectMute();
	//ここまで

	void UpdateFramePos();
};

#endif
