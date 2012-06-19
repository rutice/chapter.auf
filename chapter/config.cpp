//---------------------------------------------------------------------
//		プラグイン設定
//---------------------------------------------------------------------
#include <Windows.h>
#include <CommCtrl.h>
#include <Shlwapi.h>
#include <cstdio>
#include <string>
#include <regex>
#include <emmintrin.h>
#include "resource.h"
#include "config.h"
#include "mylib.h"

//[ru]計測クラス
//#define CHECKSPEED
#ifdef CHECKSPEED
class QPC {
	LARGE_INTEGER freq;
	LARGE_INTEGER diff;
	LARGE_INTEGER countstart;
public:
	QPC() {
		QueryPerformanceFrequency( &freq );
		diff.QuadPart = 0;
	}

	void start() {
		QueryPerformanceCounter( &countstart );
	}
	void stop() {
		LARGE_INTEGER countend;
		QueryPerformanceCounter( &countend );
		diff.QuadPart += countend.QuadPart - countstart.QuadPart;
	}

	void reset() {
		diff.QuadPart = 0;
	}

	double get() {
		return (double)(diff.QuadPart) / freq.QuadPart * 1000.;
	}
};
#else
class QPC {
public:
	QPC() {}

	void start() {}
	void stop() {}

	void reset() {}

	double get() { return 0.0;}
};
#endif
//ここまで

void CfgDlg::Init(HWND hwnd,void *editp,FILTER *fp) {
	HFONT hfont,hfont2;
	char str[STRLEN];
	HINSTANCE hinst = fp->dll_hinst;

	m_fp = fp;
	m_exfunc = fp->exfunc;
	m_scale = 30000;	// 29.97fps
	m_rate = 1001;	// 29.97fps
	m_numFrame = 0;
	m_numChapter = 0;
	m_numHis = 0;
	m_editp = NULL;
	m_hDlg = hwnd;
	m_loadfile = false;

	// フォント
	hfont2 = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	hfont = my_getfont(fp,editp);	// AviUtlデフォルトフォント

	// ウインドウの作成（部品追加）
	SendMessage(hwnd,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindowEx(WS_EX_CLIENTEDGE,"LISTBOX","",WS_CHILD|WS_VISIBLE|LBS_NOTIFY|WS_VSCROLL|WS_TABSTOP,0,0,448,335,hwnd,(HMENU)IDC_LIST1,hinst,0);
	SendDlgItemMessage(hwnd,IDC_LIST1,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","",WS_CHILD|WS_VISIBLE|ES_READONLY,48,336,190,20,hwnd,(HMENU)IDC_EDTIME,hinst,0);
	SendDlgItemMessage(hwnd,IDC_EDTIME,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindowEx(WS_EX_CLIENTEDGE,"COMBOBOX","",WS_CHILD|WS_VISIBLE|CBS_DROPDOWN|WS_VSCROLL|WS_TABSTOP,48,357,400,120,hwnd,(HMENU)IDC_EDNAME,hinst,0);
	SendDlgItemMessage(hwnd,IDC_EDNAME,WM_SETFONT,(WPARAM)hfont2,0);
	CreateWindow("BUTTON","保存",WS_CHILD|WS_VISIBLE,450,12,73,22,hwnd,(HMENU)IDC_BUSAVE,hinst,0);
	SendDlgItemMessage(hwnd,IDC_BUSAVE,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindow("BUTTON","読込",WS_CHILD|WS_VISIBLE,450,40,73,22,hwnd,(HMENU)IDC_BULOAD,hinst,0);
	SendDlgItemMessage(hwnd,IDC_BULOAD,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindow("BUTTON","自動出力",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,450,65,73,22,hwnd,(HMENU)IDC_CHECK1,hinst,0);
	SendDlgItemMessage(hwnd,IDC_CHECK1,WM_SETFONT,(WPARAM)hfont,0);
	//[ru]ボタン追加
	CreateWindow("BUTTON","無音部分",WS_CHILD|WS_VISIBLE,450,100,73,22,hwnd,(HMENU)IDC_BUDETECT,hinst,0);
	SendDlgItemMessage(hwnd,IDC_BUDETECT,WM_SETFONT,(WPARAM)hfont,0);
	
	CreateWindow("STATIC","連続",WS_CHILD|WS_VISIBLE,450,130,73,22,hwnd,(HMENU)IDC_STATICa,hinst,0);
	SendDlgItemMessage(hwnd,IDC_STATICa,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","",WS_CHILD|WS_VISIBLE,490,127,33,22,hwnd,(HMENU)IDC_EDITSERI,hinst,0);
	SendDlgItemMessage(hwnd,IDC_EDITSERI,WM_SETFONT,(WPARAM)hfont,0);

	CreateWindow("STATIC","閾値",WS_CHILD|WS_VISIBLE,450,160,73,22,hwnd,(HMENU)IDC_STATICb,hinst,0);
	SendDlgItemMessage(hwnd,IDC_STATICb,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","",WS_CHILD|WS_VISIBLE,490,157,33,22,hwnd,(HMENU)IDC_EDITMUTE,hinst,0);
	SendDlgItemMessage(hwnd,IDC_EDITMUTE,WM_SETFONT,(WPARAM)hfont,0);
	
	CreateWindow("BUTTON","SC位置",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,450,215,73,22,hwnd,(HMENU)IDC_CHECKSC,hinst,0);
	SendDlgItemMessage(hwnd,IDC_CHECKSC,WM_SETFONT,(WPARAM)hfont,0);
	//--ここまで
	CreateWindow("BUTTON","全SC検出",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,450,240,90,22,hwnd,(HMENU)IDC_PRECHECK,hinst,0);
	SendDlgItemMessage(hwnd,IDC_PRECHECK,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindow("BUTTON","mark付与",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,450,265,90,22,hwnd,(HMENU)IDC_SCMARK,hinst,0);
	SendDlgItemMessage(hwnd,IDC_SCMARK,WM_SETFONT,(WPARAM)hfont,0);


	CreateWindow("BUTTON","削除",WS_CHILD|WS_VISIBLE,450,335,73,22,hwnd,(HMENU)IDC_BUDEL,hinst,0);
	SendDlgItemMessage(hwnd,IDC_BUDEL,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindow("BUTTON","追加",WS_CHILD|WS_VISIBLE,450,360,73,22,hwnd,(HMENU)IDC_BUADD,hinst,0);
	SendDlgItemMessage(hwnd,IDC_BUADD,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindow("STATIC","時間",WS_CHILD|WS_VISIBLE,12,336,31,17,hwnd,(HMENU)IDC_STATIC1,hinst,0);
	SendDlgItemMessage(hwnd,IDC_STATIC1,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindow("STATIC","名称",WS_CHILD|WS_VISIBLE,12,360,31,17,hwnd,(HMENU)IDC_STATIC2,hinst,0);
	SendDlgItemMessage(hwnd,IDC_STATIC2,WM_SETFONT,(WPARAM)hfont,0);

	Resize();

	// tooltip
	struct {
		int id;
		char *help;
	} tips[] = {
		{ IDC_BUSAVE, "チャプターファイル（*.txt）を保存します。" },
		{ IDC_BULOAD, "チャプターファイル（*.txt）を読み込みます。画面にファイルをD&Dしても読み込めます。" },
		{ IDC_CHECK1, "エンコード開始時に、チャプター一覧を chapter.txt に書き出します。無音検索では基本的に使いません。" },
		{ IDC_BUDEL, "選択したチャプター（無音位置）を一覧から削除します。" },
		{ IDC_BUADD, "チャプター（無音位置）を一覧に追加します。無音検索では基本的に使いません。" },
		{ IDC_BUDETECT, "無音位置の検索を開始します。" },
		{ IDC_EDITSERI, "検出する最低連続無音フレーム数（5 ～）デフォルト：10" },
		{ IDC_EDITMUTE, "無音と判定する閾値（0 ～ 32767）デフォルト：50" },
		{ IDC_CHECKSC, "“無音位置へシークする時”、シーンチェンジを検出してその位置を表示します。" },
		{ IDC_PRECHECK, "「無音検索」時に、シーンチェンジ検索も併せて行います（時間がかかります）。" },
		{ IDC_SCMARK, "「全SC検索」チェック時に無音検索した場合、または「読込」時にSCPos情報がある場合、シーンチェンジ位置にマークを打ちます。" }
	};

	TOOLINFO ti;
	ZeroMemory(&ti, sizeof(ti));
	HWND htip = CreateWindowEx(0, TOOLTIPS_CLASS, NULL, TTS_ALWAYSTIP | TTS_NOPREFIX | TTS_BALLOON, 
		CW_USEDEFAULT,CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hwnd, 0, hinst, 
		NULL);
	ti.cbSize = sizeof(ti);
	ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
	ti.hwnd = hwnd;
	for(int i=0; i<sizeof(tips)/sizeof(tips[0]); i++) {
		ti.uId = (UINT_PTR)GetDlgItem(hwnd, tips[i].id);
		ti.lpszText = tips[i].help;
		SendMessage(htip , TTM_ADDTOOL , 0 , (LPARAM)&ti);
	}

	// コンボボックスに履歴追加
	for (int n=0; n<NUMHIS; n++) {
		sprintf_s(str, "history%d", n);
		m_exfunc->ini_load_str(fp, str, m_strHis[n], NULL);
		if (m_strHis[n][0] != NULL) {
			m_numHis++;
		}
	}
	AddHis();

	// 自動出力のチェックボックス
	m_autosave = m_exfunc->ini_load_int(fp,"autosave",0);
	CheckDlgButton(hwnd, IDC_CHECK1, m_autosave);

	//[ru]設定を読み込む
	int seri = m_exfunc->ini_load_int(fp, "muteCount", 10);
	sprintf_s(str, STRLEN, "%d", seri);
	SetDlgItemText(hwnd, IDC_EDITSERI, str);

	int mute = m_exfunc->ini_load_int(fp, "muteLimit", 50);
	sprintf_s(str, STRLEN, "%d", mute);
	SetDlgItemText(hwnd, IDC_EDITMUTE, str);

	CheckDlgButton(hwnd, IDC_CHECKSC, m_exfunc->ini_load_int(fp,"sceneChange", 1));
	CheckDlgButton(hwnd, IDC_PRECHECK, m_exfunc->ini_load_int(fp,"PrecheckSC", 0));
	CheckDlgButton(hwnd, IDC_SCMARK, m_exfunc->ini_load_int(fp,"SCMark", 0));
	//ここまで
}

void CfgDlg::Resize() {
	if (!m_hDlg) {
		return;
	}

	RECT rc;
	GetClientRect(m_hDlg, &rc);
	int w = rc.right;
	int h = rc.bottom;

	int left = w - 80;
	int bottom = h - 55;

	GetWindowRect(GetDlgItem(m_hDlg, IDC_LIST1), &rc);
	int oldLeft = rc.right - rc.left;
	int oldTop = rc.bottom - rc.top;

	MoveWindow(GetDlgItem(m_hDlg, IDC_LIST1), 0, 0, left, bottom, TRUE);

	GetWindowRect(GetDlgItem(m_hDlg, IDC_LIST1), &rc);
	bottom = rc.bottom - rc.top;

	// 右側
	int rightItems[] = {
		IDC_BUSAVE,
		IDC_BULOAD,
		IDC_CHECK1,
		IDC_BUDETECT,
		IDC_STATICa,
		IDC_EDITSERI,
		IDC_STATICb,
		IDC_EDITMUTE,
		IDC_CHECKSC,
		IDC_PRECHECK,
		IDC_SCMARK,
		IDC_BUDEL,
		IDC_BUADD,
		0
	};

	for (int i=0; rightItems[i]; ++i) {
		HWND hItem = GetDlgItem(m_hDlg, rightItems[i]);
		GetWindowRect(hItem, &rc);
		MapWindowPoints(NULL, m_hDlg, (LPPOINT)&rc, 2);
		MoveWindow(hItem, rc.left + left - oldLeft, rc.top, rc.right - rc.left, rc.bottom  - rc.top, TRUE);
	}

	// 下側
	int bottomItems[] = {
		IDC_EDTIME,
		IDC_EDNAME,
		IDC_STATIC1,
		IDC_STATIC2,
		IDC_BUDEL,
		IDC_BUADD,
		0,
	};

	for (int i=0; bottomItems[i]; ++i) {
		HWND hItem = GetDlgItem(m_hDlg, bottomItems[i]);
		GetWindowRect(hItem, &rc);
		MapWindowPoints(NULL, m_hDlg, (LPPOINT)&rc, 2);
		MoveWindow(hItem, rc.left, rc.top + bottom - oldTop, rc.right - rc.left, rc.bottom  - rc.top, TRUE);
	}

	// 下側の横幅
	HWND hItem = GetDlgItem(m_hDlg, IDC_EDNAME);
	GetWindowRect(hItem, &rc);
	MapWindowPoints(NULL, m_hDlg, (LPPOINT)&rc, 2);
	MoveWindow(hItem, rc.left, rc.top, left - rc.left, rc.bottom - rc.top, TRUE);
}

void CfgDlg::AutoSaveCheck() {
	m_autosave = IsDlgButtonChecked(m_fp->hwnd,IDC_CHECK1);
	m_exfunc->ini_save_int(m_fp,"autosave",m_autosave);
	//[ru]保存
	m_exfunc->ini_save_int(m_fp,"sceneChange", IsDlgButtonChecked(m_fp->hwnd, IDC_CHECKSC));
	//ここまで
	m_exfunc->ini_save_int(m_fp,"PrecheckSC", IsDlgButtonChecked(m_fp->hwnd, IDC_PRECHECK));
	m_exfunc->ini_save_int(m_fp,"SCMark", IsDlgButtonChecked(m_fp->hwnd, IDC_SCMARK));
}

void CfgDlg::SetFps(int rate,int scale) {
	m_scale = scale;
	m_rate = rate;
	m_loadfile = true;
	m_numChapter = 0;
	ShowList();
}

void CfgDlg::ShowList(int nSelect) {
	char str[STRLEN+50];

	SendDlgItemMessage(m_hDlg, IDC_LIST1, LB_RESETCONTENT, 0L, 0L);

	for(int n = 0;n < m_numChapter;n++) {
		std::string time_str = frame2time(m_Frame[n], m_rate, m_scale);
		sprintf_s(str,STRLEN,"%02d %06d [%s] %s\n",n + 1,m_Frame[n],time_str.c_str(),m_strTitle[n]);
		SendDlgItemMessage(m_hDlg,IDC_LIST1,LB_ADDSTRING,0L,(LPARAM)str);
	}

	if (nSelect != -1) {
		SendDlgItemMessage(m_hDlg, IDC_LIST1, LB_SETTOPINDEX, (WPARAM)max(nSelect-3, 0), 0L);
		SendDlgItemMessage(m_hDlg, IDC_LIST1, LB_SETCURSEL, (WPARAM)nSelect, 0L);
	}
}

void CfgDlg::AddHis() {
	char str[STRLEN];

	GetDlgItemText(m_hDlg,IDC_EDNAME,str,STRLEN);

	//履歴に同じ文字列があれば履歴から削除
	for(int n = 0;n < m_numHis;n++) {
		if(strcmp(str,m_strHis[n]) == 0) {
			for(int i = n;i < m_numHis - 1;i++) memcpy(m_strHis[i],m_strHis[i+1],STRLEN);
			m_strHis[m_numHis - 1][0] = 0;
			m_numHis--;
		}
	}

	//履歴に追加
	if(str[0] != 0) {
		for(int n = NUMHIS - 1 ;n > 0;n--) memcpy(m_strHis[n],m_strHis[n-1],STRLEN);
		strcpy_s(m_strHis[0], str);
		m_numHis++;
	}
	if(m_numHis > NUMHIS) m_numHis = NUMHIS;

	//コンボボックスの表示更新
	SendDlgItemMessage(m_hDlg,IDC_EDNAME,CB_RESETCONTENT,0,0);
	for(int n = 0;n < m_numHis;n++) {
		SendDlgItemMessage(m_hDlg,IDC_EDNAME,CB_ADDSTRING,0L,(LPARAM)m_strHis[n]);
	}

	//iniに履歴を保存
	for(int n = 0;n < NUMHIS;n++) {
		sprintf_s(str, "history%d", n);
		m_exfunc->ini_save_str(m_fp, str, m_strHis[n]);
	}
}

void CfgDlg::AddList() {
	char str[STRLEN];
	int ins;

	if(m_loadfile == false) return;	//ファイルが読み込まれていない
	if(m_numChapter >= MAXCHAPTER) {
		return;
	}

	GetDlgItemText(m_hDlg,IDC_EDNAME,str,STRLEN);
	//if(str[0] == '\0') return;	//タイトルが入力されてなくてもおｋ(r13)

	for(ins = 0;ins < m_numChapter;ins++) {
		if(m_Frame[ins] == m_frame) {
			//タイムコードが重複しているときは、タイトルを変更(r13)
			strcpy_s(m_strTitle[ins], STRLEN, str);
			ShowList(ins);
			AddHis();
			return;
		}
		if(m_Frame[ins] > m_frame) break;
	}
	for(int n = m_numChapter;n > ins;n--) {
		m_Frame[n] = m_Frame[n-1];
		strcpy_s(m_strTitle[n],STRLEN,m_strTitle[n-1]);
		m_SCPos[n] = m_SCPos[n-1];
	}
	m_numChapter++;

	strcpy_s(m_strTitle[ins],STRLEN,str);
	m_Frame[ins] = m_frame;
	m_SCPos[ins] = -1;
	ShowList(ins);
	AddHis();
}

void CfgDlg::DelList() {
	LRESULT sel;

	sel = SendDlgItemMessage(m_hDlg,IDC_LIST1,LB_GETCURSEL,0,0);
	if(sel == LB_ERR) return;

	if(m_loadfile == false) return;	//ファイルが読み込まれていない
	if(m_numChapter <= sel) return; //アイテムがない

	m_numChapter--;
	for(int n = sel;n < m_numChapter;n++) {
		m_Frame[n] = m_Frame[n+1];
		strcpy_s(m_strTitle[n],STRLEN,m_strTitle[n+1]);
		m_SCPos[n] = m_SCPos[n+1];
	}
	ShowList(min(sel, m_numChapter - 1));
}

void CfgDlg::NextList() {
	if(m_loadfile == false){
		return;	//ファイルが読み込まれていない
	}
	LRESULT sel = SendDlgItemMessage(m_hDlg, IDC_LIST1, LB_GETCURSEL, 0, 0);
	if(sel == LB_ERR) {
		return;
	}
	if (sel + 1 < m_numChapter) {
		SendDlgItemMessage(m_hDlg, IDC_LIST1, LB_SETCURSEL, sel+1, 0);
		Seek();
	}
}

void CfgDlg::PrevList() {
	if(m_loadfile == false){
		return;	//ファイルが読み込まれていない
	}
	LRESULT sel = SendDlgItemMessage(m_hDlg, IDC_LIST1, LB_GETCURSEL, 0, 0);
	if(sel == LB_ERR) {
		return;
	}
	if (0 < sel) {
		SendDlgItemMessage(m_hDlg, IDC_LIST1, LB_SETCURSEL, sel-1, 0);
		Seek();
	}
}

void CfgDlg::NextHereList() {
	if(!m_loadfile || !m_numChapter){
		return;	//ファイルが読み込まれていない or チャプターがない
	}
	int frame = m_exfunc->get_frame(m_editp);
	for(int i=0; i<m_numChapter; ++i) {
		if (frame < m_Frame[i]) {
			SendDlgItemMessage(m_hDlg, IDC_LIST1, LB_SETCURSEL, i, 0);
			Seek();
			return;
		}
	}
	// 最後にシーク
	SendDlgItemMessage(m_hDlg, IDC_LIST1, LB_SETCURSEL, -1, 0);
	m_exfunc->set_frame(m_editp, m_exfunc->get_frame_n(m_editp) - 1);
}

void CfgDlg::PrevHereList() {
	if(!m_loadfile || !m_numChapter){
		return;	//ファイルが読み込まれていない or チャプターがない
	}
	int frame = m_exfunc->get_frame(m_editp);
	for(int i=0; i<m_numChapter; ++i) {
		int seekPos = m_Frame[i];
		seekPos += m_SCPos[i] != -1 ? m_SCPos[i] : atoi(m_strTitle[i]) + 5;
		if (frame <= seekPos) {
			if (i == 0) {
				// 最初にシーク
				SendDlgItemMessage(m_hDlg, IDC_LIST1, LB_SETCURSEL, -1, 0);
				m_exfunc->set_frame(m_editp, 0);
				return;
			}
			SendDlgItemMessage(m_hDlg, IDC_LIST1, LB_SETCURSEL, i-1, 0);
			Seek();
			return;
		}
	}
	// 最後のチャプターにシーク
	SendDlgItemMessage(m_hDlg, IDC_LIST1, LB_SETCURSEL, m_numChapter - 1, 0);
	Seek();
}

//[ru]IIR_3DNRより拝借
//---------------------------------------------------------------------
//		輝度を8ビットにシフトする関数
//---------------------------------------------------------------------
void shift_to_eight_bit( PIXEL_YC* ycp, unsigned char* luma, int w, int max_w, int h )
{
	int skip = max_w-w;
	for(int y=0;y<h;y++){
		for(int x=0;x<w;x++) {
			if(ycp->y & 0xf000)
				*luma = (ycp->y < 0) ? 0 : 255;
			else
				*luma = ycp->y >>4 ;
			ycp++;
			luma++;
		}
		ycp += skip;
	}
}
void shift_to_eight_bit_sse( PIXEL_YC* ycp, unsigned char* luma, int w, int max_w, int h )
{
	int skip = max_w-w;
	__m128i m4095 = _mm_set1_epi16(4095);
	__m128i m0 = _mm_setzero_si128();
	for(int jy=0;jy<h;jy++){
		for(int jx=0;jx<w/16;jx++) {
			//for (int j=0; j<8; j++)
			//	y.m128i_i16[j] = (ycp+j)->y;
			//__m128i y = _mm_setzero_si128();
			//y = _mm_insert_epi16(y, (ycp+0)->y, 0);
			__m128i y = _mm_load_si128((__m128i*)ycp);
			y = _mm_insert_epi16(y, (ycp+1)->y, 1);
			y = _mm_insert_epi16(y, (ycp+2)->y, 2);
			y = _mm_insert_epi16(y, (ycp+3)->y, 3);
			y = _mm_insert_epi16(y, (ycp+4)->y, 4);
			y = _mm_insert_epi16(y, (ycp+5)->y, 5);
			y = _mm_insert_epi16(y, (ycp+6)->y, 6);
			y = _mm_insert_epi16(y, (ycp+7)->y, 7);
			ycp += 8;
			
			//__m128i y1 = _mm_setzero_si128();
			//y1 = _mm_insert_epi16(y1, (ycp+0)->y, 0);
			__m128i y1 = _mm_load_si128((__m128i*)ycp);
			y1 = _mm_insert_epi16(y1, (ycp+1)->y, 1);
			y1 = _mm_insert_epi16(y1, (ycp+2)->y, 2);
			y1 = _mm_insert_epi16(y1, (ycp+3)->y, 3);
			y1 = _mm_insert_epi16(y1, (ycp+4)->y, 4);
			y1 = _mm_insert_epi16(y1, (ycp+5)->y, 5);
			y1 = _mm_insert_epi16(y1, (ycp+6)->y, 6);
			y1 = _mm_insert_epi16(y1, (ycp+7)->y, 7);

			//__m128i cmp =_mm_cmpgt_epi16(y, m4095);
			//y = _mm_or_si128(y, cmp);

			__m128i cmp = _mm_cmpgt_epi16(y, m0);
			y = _mm_and_si128(y, cmp);
			y = _mm_srli_epi16(y, 4);

			cmp = _mm_cmpgt_epi16(y1, m0);
			y1 = _mm_and_si128(y1, cmp);
			y1 = _mm_srli_epi16(y1, 4);

			//for (int j=0; j<8; j++)
			//	luma[j] = y.m128i_i8[j*2];

			_mm_stream_si128((__m128i*)luma, _mm_packus_epi16(y, y1));

			//if(ycp->y & 0xf000)
			//	*luma = (ycp->y < 0) ? 0 : 255;
			//else
			//	*luma = ycp->y >>4 ;
			ycp += 8;
			luma += 16;
		}
		ycp += skip;
	}
}
#define FRAME_PICTURE	1
#define FIELD_PICTURE	2
int mvec(unsigned char* current_pix,unsigned char* bef_pix,int lx,int ly,int threshold,int pict_struct);
//ここまで

//[ru] ジャンプウィンドウ更新
BOOL CALLBACK searchJump(HWND hWnd, LPARAM lParam) {
	TCHAR buf[1024];
	TCHAR frames[2][100];
	sprintf_s(frames[0], "/ %d ]", lParam);
	sprintf_s(frames[1], "/ %d ]", lParam-1);
	if (GetWindowText(hWnd, buf, 1024)) {
		// まずジャンプウィンドウを探す
		if (strncmp(buf, "ジャンプウィンドウ", 18) == 0) {
			// 次に総フレーム数が一致しているのを探す
			if (strstr(buf, frames[0]) || strstr(buf, frames[1])) {
				// みっけた
				if (IsWindowVisible(hWnd))
					PostMessage(hWnd, WM_COMMAND, 0x9c6b, 0); // lParamはなんだっけ・・・
				return FALSE;
			}
		}
	}
	return TRUE;
}
//ここまで

class CThreadProc
{
protected:
	HANDLE hThread;
	HANDLE hNotify[2];
	volatile bool bTerminate;

	//処理用パラメータ
	unsigned char* pix1;
	unsigned char* pix0;
	int w, fi_w, h;
	volatile PIXEL_YC* yc;
	int movtion_vector;
public:
	CThreadProc(){
		hThread = NULL;
		hNotify[0] = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		hNotify[1] = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		bTerminate = false;
	}
	~CThreadProc(){
		::CloseHandle(hThread);
		::CloseHandle(hNotify[0]);
		::CloseHandle(hNotify[1]);
	}
	void SetParam(unsigned char* p0_, unsigned char* p1_, int w_, int fi_w_, int h_){
		pix0 = p0_;
		pix1 = p1_;
		w = w_;
		fi_w = fi_w_;
		h = h_;
	}
	void SetInputImage(PIXEL_YC* pImage){
		yc = pImage;
	}

	void Run(){
		shift_to_eight_bit_sse((PIXEL_YC*)yc, pix1, w, fi_w, h);
		movtion_vector = mvec( pix1, pix0, w, h, (100-0)*(100/FIELD_PICTURE), FIELD_PICTURE);
		
		unsigned char *tmp = pix0;
		pix0 = pix1;
		pix1 = tmp;
	}
	static DWORD WINAPI ThreadProc(void* pParam){
		CThreadProc* pThis = (CThreadProc*)pParam;
		::WaitForSingleObject(pThis->hNotify[1], INFINITE);
		while(!pThis->bTerminate){
			pThis->Run();
			::SetEvent(pThis->hNotify[0]);
			::WaitForSingleObject(pThis->hNotify[1], INFINITE);
		}
		return 0;
	}
	void StartThread(){
		DWORD threadID;
		hThread = ::CreateThread(NULL, 0, ThreadProc, (void*)this, 0, &threadID);
	}
	void ResumeThread(){
		::SetEvent(hNotify[1]);
	}
	void WaitThread(){
		::WaitForSingleObject(hNotify[0], INFINITE);
	}
	void Terminate(){
		bTerminate = true;
		ResumeThread();
		::WaitForSingleObject(hThread, INFINITE);
	}
	int GetMovtionVector(){
		return movtion_vector;
	}
};

int CfgDlg::GetSCPos(int moveto, int frames)
{
	FILE_INFO fi;
	if(!m_editp || !m_exfunc->get_source_file_info(m_editp, &fi, 0)){
		return 0;
	}

	int w = fi.w & 0xFFF0;
	int h = fi.h & 0xFFF0;

	int max_motion = -1;
	int max_motion_frame = 0;

	// 動きベクトルが最大値のフレームを検出
	unsigned char* pix1 = (unsigned char*)_aligned_malloc(w*h, 32);	//8ビットにシフトした現フレームの輝度が代入される
	unsigned char* pix0 = (unsigned char*)_aligned_malloc(w*h, 32);	//8ビットにシフトした前フレームの輝度が代入される

	//計測タイマ
	QPC totalQPC;

	totalQPC.start();

	PIXEL_YC *yc0 = (PIXEL_YC*)m_exfunc->get_ycp_source_cache(m_editp, max(moveto-1, 0), 0);
	shift_to_eight_bit_sse(yc0, pix0, w, fi.w, h);
	PIXEL_YC *yc1 = (PIXEL_YC*)m_exfunc->get_ycp_source_cache(m_editp, moveto, 0);

	CThreadProc thread;
	thread.SetParam(pix0, pix1, w, fi.w, h);
	thread.SetInputImage(yc1);
	thread.StartThread();

	int checkFrame = min(frames+5,200); 
	for (int i=0; i<checkFrame; i++) {
		thread.ResumeThread();
		PIXEL_YC *yc1 = NULL;
		if((i+1) <checkFrame){
			yc1 = (PIXEL_YC*)m_exfunc->get_ycp_source_cache(m_editp, moveto+i+1, 0);
		}
		thread.WaitThread();
		thread.SetInputImage(yc1);

		int movtion_vector = thread.GetMovtionVector();
		if (movtion_vector > max_motion) {
			max_motion = movtion_vector;
			max_motion_frame = i;
		}
	}
	thread.Terminate();
	_aligned_free(pix1);
	_aligned_free(pix0);

	totalQPC.stop();
#ifdef CHECKSPEED
	sprintf_s(str, "total: %.03f", totalQPC.get());
	MessageBox(NULL, str, NULL, 0);
	//OutputDebugString(str);
#endif

	return max_motion_frame;
}

void CfgDlg::Seek() {
	LRESULT sel;
	sel = SendDlgItemMessage(m_hDlg,IDC_LIST1,LB_GETCURSEL,0,0);
	if(sel == LB_ERR) return;
	if(m_Frame[sel] == m_frame) return;

	//[ru] シーンチェンジ検出
	int frames = atoi(m_strTitle[sel]);
	int moveto = m_Frame[sel];

	if (IsDlgButtonChecked(m_fp->hwnd, IDC_CHECKSC) && frames > 0) {	
		int max_motion_frame = m_SCPos[sel];
		if(max_motion_frame == -1){
			max_motion_frame = GetSCPos(moveto, frames);
		}
		moveto += max_motion_frame;
	}
	m_exfunc->set_frame(m_editp, moveto);
	SetDlgItemText(m_hDlg,IDC_EDNAME,m_strTitle[sel]);
	EnumWindows(searchJump, (LPARAM)m_exfunc->get_frame_n(m_editp));
}

void CfgDlg::SetFrameN(void *editp,int frame_n) {
	m_numFrame = frame_n;
	m_editp = editp;
	if (frame_n == 0) {
		m_loadfile = false;
	}
}

void CfgDlg::SetFrame(int frame) {
	std::string time_str = frame2time(frame, m_rate, m_scale);
	sprintf_s(m_strTime, "%s / %06d frame", time_str.c_str(), frame);
	SetDlgItemText(m_hDlg, IDC_EDTIME, m_strTime);
	m_frame = frame;
}

void CfgDlg::Save() {
	if(!m_numChapter) {
		return;
	}

	char path[_MAX_PATH];
	OPENFILENAME of;
	ZeroMemory(&of, sizeof(OPENFILENAME));
	ZeroMemory(path, sizeof(path));
		
	of.lStructSize = sizeof(OPENFILENAME);
	of.hwndOwner = m_hDlg;
	of.lpstrFile = path;
	of.nMaxFile = sizeof(path);
	of.lpstrFilter = "TXT (*.txt)\0*.txt";
	of.nFilterIndex = 0;
	of.lpstrFileTitle = NULL;
	of.nMaxFileTitle = 0;
	of.lpstrInitialDir = NULL;
	of.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
	if (GetSaveFileName(&of) == 0) {
		return;
	} 

	if (*PathFindExtension(path) == '\0') {
		strcat_s(path, ".txt");
	}

	if (PathFileExists(path)) {
		BOOL ret = MessageBox(m_hDlg, "ファイルを上書きしますか？", "チャプター編集", MB_YESNO|MB_ICONINFORMATION);
		if(ret != IDYES) {
			return;
		}
	}

	SaveToFile(path);
}

void CfgDlg::AutoSave() {
	if (!m_numChapter || !m_autosave) {
		return;
	}

	char path[_MAX_PATH];
	my_getexepath(path, sizeof(path));
	strcat_s(path, sizeof(path), "chapter.txt");

	SaveToFile(path);
}

bool CfgDlg::SaveToFile(const char *lpFile) {
	char str[STRLEN+100];
	FILE *file;
	if (fopen_s(&file, lpFile, "w")) {
		MessageBox(m_hDlg, "出力ファイルを開けませんでした。", "チャプター編集", MB_OK|MB_ICONINFORMATION);
		return false;
	}

	for(int n = 0;n < m_numChapter;n++) {
		std::string time_str = frame2time(m_Frame[n], m_rate, m_scale);
		sprintf_s(str, "CHAPTER%02d=%s\n", n + 1, time_str.c_str());
		fputs(str, file);
		sprintf_s(str, "CHAPTER%02dNAME=%s\n", n + 1, m_strTitle[n]);
		fputs(str, file);
	}
	fclose(file);
	return true;
}

void CfgDlg::Load() {
	char path[_MAX_PATH];
	OPENFILENAME of;

	if(m_loadfile == false) return;

	ZeroMemory(&of,sizeof(OPENFILENAME));
	ZeroMemory(path,sizeof(path));
		
	of.lStructSize = sizeof(OPENFILENAME);
	of.hwndOwner = m_hDlg;
	of.lpstrFile = path;
	of.nMaxFile = sizeof(path);
	of.lpstrFilter = "TXT (*.txt)\0*.txt";
	of.nFilterIndex = 0;
	of.lpstrFileTitle = NULL;
	of.nMaxFileTitle = 0;
	of.lpstrInitialDir = NULL;
	of.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
	if(GetOpenFileName(&of) == 0) {
		return;
	}
	LoadFromFile(path);
}

void CfgDlg::LoadFromFile(char *filename) {
	FILE *file;
	char str[STRLEN+2];
	int h,m,s,ms;
	int frame;
	if(fopen_s(&file,filename,"r")) {
		MessageBox(NULL,"ファイルを開けませんでした。","チャプター編集",MB_OK|MB_ICONINFORMATION);
		return;
	}

	const std::tr1::basic_regex<TCHAR> re1("^CHAPTER(\\d\\d\\d?)=(\\d\\d):(\\d\\d):(\\d\\d)\\.(\\d\\d\\d)");
	const std::tr1::basic_regex<TCHAR> re2("^CHAPTER(\\d\\d\\d?)NAME=(.*)$");

	m_numChapter = 0;

	while(true) {
		// 時間の処理
		if(fgets(str,STRLEN,file) == NULL) break;
		//                       0123456789012345678901
		if(strlen(str) < sizeof("CHAPTER00=00:00:00.000")) break;

		std::tr1::match_results<std::string::const_iterator> results;
		std::string stds(str);
		if (std::tr1::regex_search(stds, results, re1) == FALSE) {
			break;
		}
		h = atoi(results.str(2).c_str());
		m = atoi(results.str(3).c_str());
		s = atoi(results.str(4).c_str());
		ms = atoi(results.str(5).c_str());
		frame = time2frame(h, m, s, ms, m_rate, m_scale);

		// 名前の処理
		if(fgets(str,STRLEN,file) == NULL) break;
		//                       01234567890123
		if(strlen(str) < sizeof("CHAPTER00NAME=")) break;

		// strip
		for(int i = 0;i < STRLEN;i++) {
			if(str[i] == '\n' || str[i] == '\r') {
				str[i] = '\0'; break;
			}
		}

		stds = str;
		if (std::tr1::regex_search(stds, results, re2) == FALSE) {
			break;
		}
		
		m_Frame[m_numChapter] = frame;
		m_SCPos[m_numChapter] = -1;
		strcpy_s(m_strTitle[m_numChapter], results.str(2).c_str());

		// SC位置情報の取得
		const char *szSCPosMark = "SCPos:";
		char *pscpos = strstr(m_strTitle[m_numChapter], szSCPosMark);
		if (pscpos) {
			pscpos += strlen(szSCPosMark);
			int scFrame = atoi(pscpos);
			m_SCPos[m_numChapter] = scFrame - frame;
			
			// マーク付与
			if (IsDlgButtonChecked(m_fp->hwnd, IDC_SCMARK)){
				FRAME_STATUS frameStatus;
				m_exfunc->get_frame_status(m_editp, scFrame, &frameStatus);
				frameStatus.edit_flag |= EDIT_FRAME_EDIT_FLAG_MARKFRAME;
				m_exfunc->set_frame_status(m_editp, scFrame, &frameStatus);
			}
		}
		
		m_numChapter++;
		if(m_numChapter >= MAXCHAPTER) break;
	}
	fclose(file);
	ShowList();
}

// FAWチェックと、FAWPreview.aufを使っての1フレームデコード
class CFAW {
	bool is_half;

	bool load_failed;
	HMODULE _h;

	typedef int (__stdcall *ExtractDecode1FAW)(const short *in, int samples, short *out, bool is_half);
	ExtractDecode1FAW _ExtractDecode1FAW;

	bool load() {
		if (_ExtractDecode1FAW == NULL && load_failed == false) {
			_h = LoadLibrary("FAWPreview.auf");
			if (_h == NULL) {
				load_failed = true;
				return false;
			}
			_ExtractDecode1FAW = (ExtractDecode1FAW)GetProcAddress(_h, "ExtractDecode1FAW");
			if (_ExtractDecode1FAW == NULL) {
				FreeLibrary(_h);
				_h = NULL;
				load_failed = true;
				return false;
			}
			return true;
		}
		return true;
	}
public:
	CFAW() : _h(NULL), _ExtractDecode1FAW(NULL), load_failed(false), is_half(false) { }
	
	~CFAW() {
		if (_h) {
			FreeLibrary(_h);
		}
	}

	bool isLoadFailed(void) {
		return load_failed;
	}

	// FAW開始地点を探す。1/2なFAWが見つかれば、以降はそれしか探さない。
	// in: get_audio()で得た音声データ
	// samples: get_audio() * ch数
	// 戻り値：FAW開始位置のインデックス。なければ-1
	int findFAW(short *in, int samples) {
		// search for 72 F8 1F 4E 07 01 00 00
		static unsigned char faw11[] = {0x72, 0xF8, 0x1F, 0x4E, 0x07, 0x01, 0x00, 0x00};
		if (is_half == false) {
			for (int j=0; j<samples - 30; ++j) {
				if (memcmp(in+j, faw11, sizeof(faw11)) == 0) {
					return j;
				}
			}
		}

		// search for 00 F2 00 78 00 9F 00 CE 00 87 00 81 00 80 00 80
		static unsigned char faw12[] = {0x00, 0xF2, 0x00, 0x78, 0x00, 0x9F, 0x00, 0xCE,
										0x00, 0x87, 0x00, 0x81, 0x00, 0x80, 0x00, 0x80};

		for (int j=0; j<samples - 30; ++j) {
			if (memcmp(in+j, faw12, sizeof(faw12)) == 0) {
				is_half = true;
				return j;
			}
		}

		return -1;
	}

	// FAWPreview.aufを使ってFAWデータ1つを抽出＆デコードする
	// in: FAW開始位置のポインタ。findFAWに渡したin + findFAWの戻り値
	// samples: inにあるデータのshort換算でのサイズ
	// out: デコード結果を入れるバッファ(16bit, 2chで1024サンプル)
	//     （1024sample * 2byte * 2ch = 4096バイト必要）
	int decodeFAW(const short *in, int samples, short *out){
		if (load()) {
			return _ExtractDecode1FAW(in, samples, out, is_half);
		}
		return 0;
	}
};

class CMute {
private:
	short _buf[48000];

public:
	CMute() {}
	~CMute() {}

	static bool isMute(short *buf, int naudio, short mute) {
		for (int j=0; j<naudio; ++j) {
			if (abs(buf[j]) > mute) {
				return false;
			}
		}
		return true;
	}
};

//[ru]無音部分検出
void CfgDlg::DetectMute() {
	if(m_loadfile == false)
		return;	//ファイルが読み込まれていない

	FILE_INFO fip;
	if (!m_exfunc->get_file_info(m_editp, &fip))
		return; // 情報を取得できない

	if ((fip.flag & FILE_INFO_FLAG_AUDIO) == 0) {
		MessageBox(m_hDlg, "音声トラックがありません", NULL, MB_OK);
		return;
	}

	int seri = GetDlgItemInt(m_hDlg, IDC_EDITSERI, NULL, FALSE);
	int mute = GetDlgItemInt(m_hDlg, IDC_EDITMUTE, NULL, FALSE);

	if (seri < 5) {
		MessageBox(m_hDlg, "無音連続フレーム数は 5 以上を指定してください", NULL, MB_OK);
		return;
	}
	if (mute < 0 || 1 << 15 < mute) {
		MessageBox(m_hDlg, "無音閾値は 0 ～ 2^15 の範囲で指定してください", NULL, MB_OK);
		return;
	}
	
	m_exfunc->ini_save_int(m_fp, "muteCount", seri);
	m_exfunc->ini_save_int(m_fp, "muteLimit", mute);

	int n = m_exfunc->get_frame_n(m_editp);

	// チャプター個数
	int pos = 0;

	// 適当にでかめにメモリ確保
	short buf[48000*2];
	FRAME_STATUS fs;
	int bvid = -10;
	if (m_exfunc->get_frame_status(m_editp, 0, &fs))
		bvid = fs.video;

	int start_fr = 0;	// 無音の開始フレーム
	int mute_fr = 0;	// 無音フレーム数
	bool isFAW = false;	// FAW使用かどうか（最初の200フレームで検出）
	CFAW cfaw;

	// FAWチェック
	int fawCount = 0;
	for (int i=0; i<min(200, n); ++i) {
		int naudio = m_exfunc->get_audio_filtered(m_editp, i, buf);
		if (naudio) {
			naudio *= fip.audio_ch;
			int j = cfaw.findFAW(buf, naudio);
			if (j != -1) {
				fawCount++;
			}
		}
	}
	if (fawCount > 10) {
		isFAW = true;
	}

	// フレームごとに音声を解析
	int skip = 0;
	for (int i=0; i<n; ++i) {
		// 音声とフレームステータス取得
		if (!m_exfunc->get_frame_status(m_editp, i, &fs)) {
			continue;
		}

		// 編集点を検出
		int diff = fs.video - bvid;
		bvid = fs.video;
		if (diff && diff != 1) {
			if (diff & 0xff000000)
				sprintf_s(m_strTitle[pos], STRLEN, "ソースチェンジ");
			else
				sprintf_s(m_strTitle[pos], STRLEN, "編集点 (間隔：%d)", diff);
			m_Frame[pos] = i;
			m_SCPos[pos] = -1;
			++pos;
			mute_fr = 0;
			start_fr = i;
			continue;
		}

		if (skip) {
			skip--;
			continue;
		}

		// 先フレームを読んで音があれば飛ばす
		if (i && mute_fr == 0 ) {
			int naudio = m_exfunc->get_audio_filtered(m_editp, i + seri - 1, buf);
			if (naudio && isFAW) {
				naudio *= fip.audio_ch;
				int j = cfaw.findFAW(buf, naudio);
				if (j != -1) {
					naudio = cfaw.decodeFAW(buf + j, naudio - j, buf);
				}
			}
			if (naudio) {
				if (!CMute::isMute(buf, naudio, mute)) {
					skip = seri - 1;
					continue;
				}
			}
		}
		
		int naudio = m_exfunc->get_audio_filtered(m_editp, i, buf);
		if (naudio == 0)
			continue;

		// ch数で調整
		naudio *= fip.audio_ch;

		// FAWをデコード
		if (isFAW) {
			int j = cfaw.findFAW(buf, naudio);
			if (j != -1) {
				naudio = cfaw.decodeFAW(buf+j, naudio-j, buf);

				if (cfaw.isLoadFailed()) {
					MessageBox(this->m_fp->hwnd, "FAWをデコードするのに 11/02/06以降のFAWPreview.auf（FAWぷれびゅ～） が必要です。", "エラー", MB_OK);
					return ;
				}
			}
		}

		if (CMute::isMute(buf, naudio, mute)) {
			// 無音フレームだった
			if (mute_fr == 0) {
				start_fr = i;
			}
			++mute_fr;
		} else {
			// 無音じゃなかった
			// 基準フレーム数以上連続無音だったら
			if (mute_fr >= seri) {
				// 前回との差分が14～16秒だったら★をつける
				char *mark = "";
				if (pos > 0 && abs(start_fr - m_Frame[pos-1] - 30*15) < 30) {
					mark = "★";
				} else if (pos > 0 && abs(start_fr - m_Frame[pos-1] - 30*30) < 30) {
					mark = "★★";
				} else if (pos > 0 && abs(start_fr - m_Frame[pos-1] - 30*45) < 30) {
					mark = "★★★";
				} else if (pos > 0 && abs(start_fr - m_Frame[pos-1] - 30*60) < 30) {
					mark = "★★★★";
				}
				
				if (pos && (start_fr - m_Frame[pos-1] <= 1)) {
					sprintf_s(m_strTitle[pos-1], STRLEN, "%s 無音%02dフレーム %s", m_strTitle[pos-1], mute_fr, mark);
				} else {
					sprintf_s(m_strTitle[pos], STRLEN, "%02dフレーム %s", mute_fr, mark);
					m_Frame[pos] = start_fr;

					if (IsDlgButtonChecked(m_fp->hwnd, IDC_PRECHECK)){
						m_SCPos[pos] = GetSCPos(start_fr, mute_fr);
						sprintf_s(m_strTitle[pos], STRLEN, "%02dフレーム %s SCPos:%d", mute_fr, mark, m_SCPos[pos]);
						if (IsDlgButtonChecked(m_fp->hwnd, IDC_SCMARK)){
							int target_frame = start_fr + m_SCPos[pos];
							FRAME_STATUS frameStatus;
							m_exfunc->get_frame_status(m_editp, target_frame, &frameStatus);
							frameStatus.edit_flag |= EDIT_FRAME_EDIT_FLAG_MARKFRAME;
							m_exfunc->set_frame_status(m_editp, target_frame, &frameStatus);
						}
					}else{
						m_SCPos[pos] = -1;
					}

					++pos;
				}
			}
			mute_fr = start_fr = 0;
		}

		// 最大数オーバー
		if (pos >= MAXCHAPTER) {
			break;
		}
	}
	
	m_numChapter = pos;
	ShowList();
}
//ここまで

void CfgDlg::UpdateFramePos()
{
	int stFrame, edFrame;
	m_exfunc->get_select_frame(m_editp, &stFrame, &edFrame);
	int diff = edFrame - stFrame + 1;

	int nShowing, toSelect = -1;
	nShowing = m_exfunc->get_frame(m_editp);

	int orgNum = m_numChapter;
	int orgFrame[MAXCHAPTER];
	char orgTitle[MAXCHAPTER][STRLEN];
	int orgSCPos[MAXCHAPTER];
	memcpy(orgFrame, m_Frame, sizeof(int)*MAXCHAPTER);
	memcpy(orgTitle, m_strTitle,  sizeof(char)*MAXCHAPTER*STRLEN);
	memcpy(orgSCPos, m_SCPos, sizeof(int)*MAXCHAPTER);

	m_numChapter = 0;
	int pos = 0; // 新しい位置
	int bCutInserted = false;
	for(int n=0; n<orgNum && pos < MAXCHAPTER; n++){
		if(stFrame <= orgFrame[n] && orgFrame[n] <= edFrame){
			if (pos == 0) {
				continue;
			}
			if (bCutInserted == false) {
				bCutInserted = true;
				if (m_Frame[pos-1] + 30 > stFrame) {
					pos--;
				}
				m_Frame[pos] = stFrame;
				m_SCPos[pos] = 0;
				sprintf_s(m_strTitle[pos], STRLEN, "編集点 (間隔：%d)", diff);
				pos++;
			}
			continue;
		}
		// 選択位置の決定
		if (nShowing > orgFrame[n]) {
			toSelect = n;
		}

		m_Frame[pos] = orgFrame[n];
		if(orgFrame[n] > edFrame){
			m_Frame[pos] -= diff;
		}
		strcpy_s(m_strTitle[pos], STRLEN, orgTitle[n]);
		m_SCPos[pos] = orgSCPos[n];
		pos++;
	}
	m_numChapter = pos;
	ShowList(toSelect);
}
