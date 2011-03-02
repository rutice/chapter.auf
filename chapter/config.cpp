//---------------------------------------------------------------------
//		プラグイン設定
//---------------------------------------------------------------------
#include <windows.h>
#include <commctrl.h>
#include <cstdio>
#include <process.h>
#include <time.h>	//[ru]追加
#include <emmintrin.h> //[ru] 追加
#include "resource.h"
#include "config.h"
#include "mylib.h"

#include "neaacdec.h"
#ifdef _DEBUG
//#pragma comment(lib, "libfaadD.lib")
#else
//#pragma comment(lib, "libfaad.lib")
#endif

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
	m_scale = 100;	// 29.97fps
	m_rate = 2997;	// 29.97fps
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
	CreateWindowEx(WS_EX_CLIENTEDGE,"LISTBOX","",WS_CHILD|WS_VISIBLE|LBS_NOTIFY|WS_VSCROLL|WS_TABSTOP,14,12,448,335,hwnd,(HMENU)IDC_LIST1,hinst,0);
	SendDlgItemMessage(hwnd,IDC_LIST1,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","",WS_CHILD|WS_VISIBLE|ES_READONLY,48,348,190,20,hwnd,(HMENU)IDC_EDTIME,hinst,0);
	SendDlgItemMessage(hwnd,IDC_EDTIME,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindowEx(WS_EX_CLIENTEDGE,"COMBOBOX","",WS_CHILD|WS_VISIBLE|CBS_DROPDOWN|WS_VSCROLL|WS_TABSTOP,48,380,417,120,hwnd,(HMENU)IDC_EDNAME,hinst,0);
	SendDlgItemMessage(hwnd,IDC_EDNAME,WM_SETFONT,(WPARAM)hfont2,0);
	CreateWindow("BUTTON","保存",WS_CHILD|WS_VISIBLE,480,12,73,22,hwnd,(HMENU)IDC_BUSAVE,hinst,0);
	SendDlgItemMessage(hwnd,IDC_BUSAVE,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindow("BUTTON","読込",WS_CHILD|WS_VISIBLE,480,51,73,22,hwnd,(HMENU)IDC_BULOAD,hinst,0);
	SendDlgItemMessage(hwnd,IDC_BULOAD,WM_SETFONT,(WPARAM)hfont,0);
	//[ru]ボタン追加
	CreateWindow("BUTTON","無音部分",WS_CHILD|WS_VISIBLE,480,130,73,22,hwnd,(HMENU)IDC_BUDETECT,hinst,0);
	SendDlgItemMessage(hwnd,IDC_BUDETECT,WM_SETFONT,(WPARAM)hfont,0);
	
	CreateWindow("STATIC","連続",WS_CHILD|WS_VISIBLE,480,160,73,22,hwnd,(HMENU)IDC_STATICa,hinst,0);
	SendDlgItemMessage(hwnd,IDC_STATICa,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","",WS_CHILD|WS_VISIBLE,520,160,33,22,hwnd,(HMENU)IDC_EDITSERI,hinst,0);
	SendDlgItemMessage(hwnd,IDC_EDITSERI,WM_SETFONT,(WPARAM)hfont,0);

	CreateWindow("STATIC","閾値",WS_CHILD|WS_VISIBLE,480,190,73,22,hwnd,(HMENU)IDC_STATICb,hinst,0);
	SendDlgItemMessage(hwnd,IDC_STATICb,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","",WS_CHILD|WS_VISIBLE,520,190,33,22,hwnd,(HMENU)IDC_EDITMUTE,hinst,0);
	SendDlgItemMessage(hwnd,IDC_EDITMUTE,WM_SETFONT,(WPARAM)hfont,0);
	
	CreateWindow("BUTTON","SC位置",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,480,215,73,22,hwnd,(HMENU)IDC_CHECKSC,hinst,0);
	SendDlgItemMessage(hwnd,IDC_CHECKSC,WM_SETFONT,(WPARAM)hfont,0);
	//--ここまで
	CreateWindow("BUTTON","全SC検出",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,480,240,90,22,hwnd,(HMENU)IDC_PRECHECK,hinst,0);
	SendDlgItemMessage(hwnd,IDC_PRECHECK,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindow("BUTTON","mark付与",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,480,265,90,22,hwnd,(HMENU)IDC_SCMARK,hinst,0);
	SendDlgItemMessage(hwnd,IDC_SCMARK,WM_SETFONT,(WPARAM)hfont,0);


	CreateWindow("BUTTON","削除",WS_CHILD|WS_VISIBLE,480,346,73,22,hwnd,(HMENU)IDC_BUDEL,hinst,0);
	SendDlgItemMessage(hwnd,IDC_BUDEL,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindow("BUTTON","追加",WS_CHILD|WS_VISIBLE,480,377,73,22,hwnd,(HMENU)IDC_BUADD,hinst,0);
	SendDlgItemMessage(hwnd,IDC_BUADD,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindow("STATIC","時間",WS_CHILD|WS_VISIBLE,12,351,31,17,hwnd,(HMENU)IDC_STATIC1,hinst,0);
	SendDlgItemMessage(hwnd,IDC_STATIC1,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindow("STATIC","名称",WS_CHILD|WS_VISIBLE,12,384,31,17,hwnd,(HMENU)IDC_STATIC2,hinst,0);
	SendDlgItemMessage(hwnd,IDC_STATIC2,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindow("BUTTON","自動出力",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,480,90,73,22,hwnd,(HMENU)IDC_CHECK1,hinst,0);
	SendDlgItemMessage(hwnd,IDC_CHECK1,WM_SETFONT,(WPARAM)hfont,0);

	// コンボボックスに履歴追加
	for(int n = 0;n < NUMHIS;n++) {
		sprintf_s(str,STRLEN,"history%d",n);
		m_exfunc->ini_load_str(fp,str,m_strHis[n],NULL);
		if(m_strHis[n][0] != NULL) m_numHis++;
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
	LONGLONG t,h,m;
	double s;
	char str[STRLEN];

	SendDlgItemMessage(m_hDlg, IDC_LIST1, LB_RESETCONTENT, 0L, 0L);

	for(int n = 0;n < m_numChapter;n++) {
		t = (LONGLONG)m_Frame[n] * 10000000 * m_scale / m_rate;
		h = t / 36000000000;
		m = (t - h * 36000000000) / 600000000;
		s = (t - h * 36000000000 - m * 600000000) / 10000000.0;
		sprintf_s(str,STRLEN,"%02d %06d [%02d:%02d:%06.3f] %s\n",n + 1,m_Frame[n],(int)h,(int)m,s,m_strTitle[n]);
		SendDlgItemMessage(m_hDlg,IDC_LIST1,LB_ADDSTRING,0L,(LPARAM)str);
	}

	if (nSelect != -1) {
		SendDlgItemMessage(m_hDlg, IDC_LIST1, LB_SETCURSEL, (WPARAM)min(m_numChapter, nSelect + 8), 0L);
		SendDlgItemMessage(m_hDlg, IDC_LIST1, LB_SETCURSEL, (WPARAM)nSelect, 0L);
	}
}

void CfgDlg::AddHis() {
	char str[STRLEN];

	GetDlgItemText(m_hDlg,IDC_EDNAME,str,STRLEN);
	SetDlgItemText(m_hDlg,IDC_EDNAME,"");

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
		strcpy_s(m_strHis[0],STRLEN,str);
		m_numHis++;
	}
	if(m_numHis > NUMHIS) m_numHis = NUMHIS;

	//コンボボックスの表示更新
	while(SendDlgItemMessage(m_hDlg,IDC_EDNAME,CB_GETCOUNT,0,0)) {SendDlgItemMessage(m_hDlg,IDC_EDNAME,CB_DELETESTRING,0,0);}
	for(int n = 0;n < m_numHis;n++) {
		SendDlgItemMessage(m_hDlg,IDC_EDNAME,CB_ADDSTRING,0L,(LPARAM)m_strHis[n]);
	}

	//iniに履歴を保存
	for(int n = 0;n < NUMHIS;n++) {
		sprintf_s(str,STRLEN,"history%d",n);
		m_exfunc->ini_save_str(m_fp,str,m_strHis[n]);
	}
}

void CfgDlg::AddList() {
	char str[STRLEN];
	int ins;

	if(m_loadfile == false) return;	//ファイルが読み込まれていない
	if(m_numChapter > 99) return;

	GetDlgItemText(m_hDlg,IDC_EDNAME,str,STRLEN);
	if(str[0] == NULL) return;	//タイトルが入力されていない

	for(ins = 0;ins < m_numChapter;ins++) {
		if(m_Frame[ins] == m_frame) return;	//タイムコードが重複している
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
	ShowList();
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
	ShowList();
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

//[ru]輝度の平均だけで判定してみるテスト
int ave_y(PIXEL_YC *pyc, int w, int h) {
	unsigned int ave = 0;
	int skip_w = w % 4;
	w >>= 2;
	for (int i=0; i<h; i++) {
		unsigned int s = 0;
		for (int j=0; j<w; j++) {
			s += (pyc+0)->y;
			s += (pyc+1)->y;
			s += (pyc+2)->y;
			s += (pyc+3)->y;
			pyc += 4;
		}
		ave += s >> 5;
		pyc += skip_w;
	}
	return ave;
}
//ここまで

//[ru] ジャンプウィンドウ更新
BOOL searchJump(HWND hWnd, LPARAM lParam) {
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

#if 1
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

#else
#if 1
	// 動きベクトルが最大値のフレームを検出
	unsigned char* pix1 = (unsigned char*)_aligned_malloc(w*h, 32);	//8ビットにシフトした現フレームの輝度が代入される
	unsigned char* pix0 = (unsigned char*)_aligned_malloc(w*h, 32);	//8ビットにシフトした前フレームの輝度が代入される

	//計測タイマ
	QPC totalQPC;
	QPC sourceQPC;
	QPC eightQPC;
	QPC mvecQPC;

	totalQPC.start();

	sourceQPC.start();
	PIXEL_YC *yc0 = (PIXEL_YC*)m_exfunc->get_ycp_source_cache(m_editp, max(moveto-1, 0), 0);
	sourceQPC.stop();
	eightQPC.start();
	shift_to_eight_bit_sse(yc0, pix0, w, fi.w, h);
	eightQPC.stop();
	for (int i=0; i<min(frames+5,200); i++) {
		sourceQPC.start();
		PIXEL_YC *yc1 = (PIXEL_YC*)m_exfunc->get_ycp_source_cache(m_editp, moveto+i, 0);
		sourceQPC.stop();
		eightQPC.start();
		shift_to_eight_bit_sse(yc1, pix1, w, fi.w, h);
		eightQPC.stop();
		mvecQPC.start();
		int movtion_vector = mvec( pix1, pix0, w, h, (100-0)*(100/FIELD_PICTURE), FIELD_PICTURE);
		mvecQPC.stop();
		yc0 = yc1;
		if (movtion_vector > max_motion) {
			max_motion = movtion_vector;
			max_motion_frame = i;
		}
		unsigned char *tmp = pix0;
		pix0 = pix1;
		pix1 = tmp;
		//memcpy(pix0, pix1, w*h);
	}
	_aligned_free(pix1);
	_aligned_free(pix0);

	totalQPC.stop();
#ifdef CHECKSPEED
	sprintf_s(str, "total: %.03f, read: %.03f, shift: %.03f, mvec: %.03f", totalQPC.get(), sourceQPC.get(), eightQPC.get(), mvecQPC.get());
	MessageBox(NULL, str, NULL, 0);
#endif
#else
	// 輝度の変化が最大のフレームを検出
	PIXEL_YC *yc0 = (PIXEL_YC*)m_exfunc->get_ycp_source_cache(m_editp, max(moveto-1, 0), 0);
	int before_ave = yc0 != NULL ? ave_y(yc0, fi.w, fi.h) : 0;
	for (int i=0; i<min(frames+5,200); i++) {
		PIXEL_YC *yc1 = (PIXEL_YC*)m_exfunc->get_ycp_source_cache(m_editp, moveto+i, 0);
		if (yc1 == NULL)
			continue;
		int now_ave = ave_y(yc1, fi.w, fi.h);
		int movtion_vector = abs(now_ave - before_ave);
		before_ave = now_ave;

		if (movtion_vector > max_motion) {
			max_motion = movtion_vector;
			max_motion_frame = i;
		}
	}
#endif
#endif
	//sprintf_s(str, 500, "%.03f", (double)( clock() - t ) / CLOCKS_PER_SEC);
	//MessageBox(NULL, str, NULL, 0);

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
		max_motion_frame += moveto;
		m_exfunc->set_frame(m_editp, max_motion_frame);
		EnumWindows((WNDENUMPROC)searchJump, (LPARAM)m_exfunc->get_frame_n(m_editp));
		return;
	}
	//ここまで
	m_exfunc->set_frame(m_editp,m_Frame[sel]);
	SetDlgItemText(m_hDlg,IDC_EDNAME,m_strTitle[sel]);
	EnumWindows((WNDENUMPROC)searchJump, (LPARAM)m_exfunc->get_frame_n(m_editp));
}

void CfgDlg::SetFrameN(void *editp,int frame_n) {
	m_numFrame = frame_n;
	m_editp = editp;
}

void CfgDlg::SetFrame(int frame) {
	LONGLONG t,h,m;
	double s;

	t = (LONGLONG)frame * 10000000 * m_scale / m_rate;
	h = t / 36000000000;
	m = (t - h * 36000000000) / 600000000;
	s = (t - h * 36000000000 - m * 600000000) / 10000000.0;
	sprintf_s(m_strTime,STRLEN,"%02d:%02d:%06.3f / %06d frame", (int)h, (int)m, s, frame);
	SetDlgItemText(m_hDlg,IDC_EDTIME,m_strTime);
	m_frame = frame;
}

void CfgDlg::Save() {
	LONGLONG t,h,m;
	double s;
	char str[STRLEN],path[_MAX_PATH];
	FILE *file;
	OPENFILENAME of;

	if(m_numChapter == 0) return;

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
	if(GetSaveFileName(&of) == 0) return;

	bool ext = false;	// 拡張子が無い場合は付ける
	for(int n = 0;n < sizeof(path);n++) {
		if(path[n] == '\\' && !my_sjis(path,n-1)) ext = false;
		if(path[n] == '.' && !my_sjis(path,n-1)) ext = true;
		if(path[n] == 0) break;
	}
	if(ext == false) strcat_s(path,sizeof(path),".txt");

	if(fopen_s(&file,path,"r") == 0) {
		fclose(file);
		if(MessageBox(NULL,"ファイルを上書きしますか？","チャプター編集",MB_YESNO|MB_ICONINFORMATION)
			== IDCANCEL) return;
	}

	if(fopen_s(&file,path,"w")) {
		MessageBox(NULL,"ファイルを書き込めませんでした。","チャプター編集",MB_OK|MB_ICONINFORMATION);
		return;
	}

	for(int n = 0;n < m_numChapter;n++) {
		t = (LONGLONG)m_Frame[n] * 10000000 * m_scale / m_rate;
		h = t / 36000000000;
		m = (t - h * 36000000000) / 600000000;
		s = (t - h * 36000000000 - m * 600000000) / 10000000.0;
		sprintf_s(str,STRLEN,"CHAPTER%02d=%02d:%02d:%06.3f\n",n + 1,(int)h,(int)m,s);
		fputs(str,file);
		sprintf_s(str,STRLEN,"CHAPTER%02dNAME=%s\n",n + 1,m_strTitle[n]);
		fputs(str,file);
	}
	fclose(file);
}

void CfgDlg::AutoSave() {
	LONGLONG t,h,m;
	double s;
	char str[STRLEN],path[_MAX_PATH];
	FILE *file;

	if(m_numChapter == 0 || m_autosave == 0) return;

	my_getexepath(path,sizeof(path));
	strcat_s(path,sizeof(path),"chapter.txt");

	if(fopen_s(&file,path,"w")) {
		MessageBox(NULL,"自動出力ファイルを書き込めませんでした。","チャプター編集",MB_OK|MB_ICONINFORMATION);
		return;
	}

	for(int n = 0;n < m_numChapter;n++) {
		t = (LONGLONG)m_Frame[n] * 10000000 * m_scale / m_rate;
		h = t / 36000000000;
		m = (t - h * 36000000000) / 600000000;
		s = (t - h * 36000000000 - m * 600000000) / 10000000.0;
		sprintf_s(str,STRLEN,"CHAPTER%02d=%02d:%02d:%06.3f\n",n + 1,(int)h,(int)m,s);
		fputs(str,file);
		sprintf_s(str,STRLEN,"CHAPTER%02dNAME=%s\n",n + 1,m_strTitle[n]);
		fputs(str,file);
	}
	fclose(file);
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
	char str[STRLEN];
	LONGLONG t;
	int h,m,s;
	int frame;
	if(fopen_s(&file,filename,"r")) {
		MessageBox(NULL,"ファイルを開けませんでした。","チャプター編集",MB_OK|MB_ICONINFORMATION);
		return;
	}

	m_numChapter = 0;

	while(true) {
		if(fgets(str,STRLEN,file) == NULL) break;
		//                       0123456789012345678901
		if(strlen(str) < sizeof("CHAPTER00=00:00:00.000")) break;
		h = (str[10] - '0') * 10 + (str[11] - '0');
		m = (str[13] - '0') * 10 + (str[14] - '0');
		s = (str[16] - '0') * 10000 + (str[17] - '0') * 1000 + (str[19] - '0') * 100 + (str[20] - '0') * 10 + (str[21] - '0');
		t = (LONGLONG)h * 36000000000 + (LONGLONG)m * 600000000 + (LONGLONG)s * 10000;
		frame = (int)(t * m_rate / m_scale / 10000000);
		if(frame < 0) frame = 0;

		if(fgets(str,STRLEN,file) == NULL) break;
		//                       01234567890123
		if(strlen(str) < sizeof("CHAPTER00NAME=")) break;
		for(int i = 0;i < STRLEN;i++) if(str[i] == '\n' || str[i] == '\r') {str[i] = 0; break;}
		strcpy_s(m_strTitle[m_numChapter],STRLEN,str + 14);
		m_Frame[m_numChapter] = frame;
		m_SCPos[m_numChapter] = -1;

		char *scPos = strstr(m_strTitle[m_numChapter], "SCPos:");
		int scFrame;
		if (scPos) {
			scFrame = atoi(scPos + strlen("SCPos:"));
			m_SCPos[m_numChapter] = scFrame - frame;
		}

		if (IsDlgButtonChecked(m_fp->hwnd, IDC_SCMARK)){
			FRAME_STATUS frameStatus;
			m_exfunc->get_frame_status(m_editp, scFrame, &frameStatus);
			frameStatus.edit_flag |= EDIT_FRAME_EDIT_FLAG_MARKFRAME;
			m_exfunc->set_frame_status(m_editp, scFrame, &frameStatus);
		}

		m_numChapter++;
		if(m_numChapter > 100) break;
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

	/*
	sprintf_s(str, STRLEN, "音量(%d/%d)以下の部分が %d フレーム以上連続している部分を探します。\n現在のチャプター情報は全て削除されます！", mute, 1 << 15, seri);
	if (MessageBox(m_hDlg, str, "無音検索", MB_OKCANCEL) != IDOK) {
		return ;
	}
	*/

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
	bool isFAW = true;	// FAW使用かどうか（最初のフレームで検出）
	CFAW cfaw;

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
			int naudio = m_exfunc->get_audio(m_editp, i + seri - 1, buf);
			if (naudio && isFAW) {
				naudio *= fip.audio_ch;
				int j = cfaw.findFAW(buf, naudio);
				if (j != -1) {
					//byte *buffer = (byte*)(&buf[j+4]);
					//naudio = CMute::decodeFAW(buffer, 2*(naudio - j - 4), buf);
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
		
		int naudio = m_exfunc->get_audio(m_editp, i, buf);
		if (naudio == 0)
			continue;

		// ch数で調整
		naudio *= fip.audio_ch;

		// FAWをデコード
		if (isFAW) {
			bool isDecoded = false;
			int j = cfaw.findFAW(buf, naudio);
			if (j != -1) {
				//byte *buffer = (byte*)(&buf[j+4]);
				//naudio = CMute::decodeFAW(buffer, 2*(naudio - j - 4), buf);
				naudio = cfaw.decodeFAW(buf+j, naudio-j, buf);
				isDecoded = naudio != 0;

				if (cfaw.isLoadFailed()) {
					MessageBox(this->m_fp->hwnd, "FAWをデコードするのに 11/02/06以降のFAWPreview.auf（FAWぷれびゅ～） が必要です。", "エラー", MB_OK);
					return ;
				}
			}
			if (isDecoded == false) {
				// 最初のフレームでAAC部分が無ければFAWモードを抜ける
				if (i == 0)
					isFAW = false;
				else
					continue;
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
		if (pos > 99) {
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
	int orgFrame[100];
	char orgTitle[100][STRLEN];
	int orgSCPos[100];
	memcpy(orgFrame, m_Frame, sizeof(int)*100);
	memcpy(orgTitle, m_strTitle,  sizeof(char)*100*STRLEN);
	memcpy(orgSCPos, m_SCPos, sizeof(int)*100);

	m_numChapter = 0;
	int pos = 0; // 新しい位置
	int bCutInserted = false;
	for(int n=0; n<orgNum && pos < 100; n++){
		if(orgFrame[n] >= stFrame && orgFrame[n] <= edFrame){
			if (bCutInserted == false) {
				bCutInserted = true;
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
