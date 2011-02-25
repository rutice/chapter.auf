//---------------------------------------------------------------------
//		プラグイン設定
//---------------------------------------------------------------------
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <process.h>
#include "resource.h"
#include "config.h"
#include "mylib.h"

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
	CreateWindowEx(WS_EX_CLIENTEDGE,"LISTBOX","",WS_CHILD|WS_VISIBLE|LBS_NOTIFY|WS_VSCROLL|WS_TABSTOP,14,12,448,225,hwnd,(HMENU)IDC_LIST1,hinst,0);
	SendDlgItemMessage(hwnd,IDC_LIST1,WM_SETFONT,(WPARAM)hfont2,0);
	CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","",WS_CHILD|WS_VISIBLE|ES_READONLY,48,248,89,20,hwnd,(HMENU)IDC_EDTIME,hinst,0);
	SendDlgItemMessage(hwnd,IDC_EDTIME,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindowEx(WS_EX_CLIENTEDGE,"COMBOBOX","",WS_CHILD|WS_VISIBLE|CBS_DROPDOWN|WS_VSCROLL|WS_TABSTOP,48,280,417,120,hwnd,(HMENU)IDC_EDNAME,hinst,0);
	SendDlgItemMessage(hwnd,IDC_EDNAME,WM_SETFONT,(WPARAM)hfont2,0);
	CreateWindow("BUTTON","保存",WS_CHILD|WS_VISIBLE,480,12,73,22,hwnd,(HMENU)IDC_BUSAVE,hinst,0);
	SendDlgItemMessage(hwnd,IDC_BUSAVE,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindow("BUTTON","読込",WS_CHILD|WS_VISIBLE,480,51,73,22,hwnd,(HMENU)IDC_BULOAD,hinst,0);
	SendDlgItemMessage(hwnd,IDC_BULOAD,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindow("BUTTON","削除",WS_CHILD|WS_VISIBLE,480,246,73,22,hwnd,(HMENU)IDC_BUDEL,hinst,0);
	SendDlgItemMessage(hwnd,IDC_BUDEL,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindow("BUTTON","追加",WS_CHILD|WS_VISIBLE,480,277,73,22,hwnd,(HMENU)IDC_BUADD,hinst,0);
	SendDlgItemMessage(hwnd,IDC_BUADD,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindow("STATIC","時間",WS_CHILD|WS_VISIBLE,12,251,31,17,hwnd,(HMENU)IDC_STATIC1,hinst,0);
	SendDlgItemMessage(hwnd,IDC_STATIC1,WM_SETFONT,(WPARAM)hfont,0);
	CreateWindow("STATIC","名称",WS_CHILD|WS_VISIBLE,12,284,31,17,hwnd,(HMENU)IDC_STATIC2,hinst,0);
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
}

void CfgDlg::AuotSaveCheck() {
	m_autosave = IsDlgButtonChecked(m_fp->hwnd,IDC_CHECK1);
	m_exfunc->ini_save_int(m_fp,"autosave",m_autosave);
}

void CfgDlg::SetFps(int rate,int scale) {
	m_scale = scale;
	m_rate = rate;
	m_loadfile = true;
	m_numChapter = 0;
	ShowList();
}

void CfgDlg::ShowList() {
	LONGLONG t,h,m;
	double s;
	char str[STRLEN];

	while(SendDlgItemMessage(m_hDlg,IDC_LIST1,LB_GETCOUNT,0,0)) {SendDlgItemMessage(m_hDlg,IDC_LIST1,LB_DELETESTRING,0,0);}

	for(int n = 0;n < m_numChapter;n++) {
		t = (LONGLONG)m_Frame[n] * 10000000 * m_scale / m_rate;
		h = t / 36000000000;
		m = (t - h * 36000000000) / 600000000;
		s = (t - h * 36000000000 - m * 600000000) / 10000000.0;
		sprintf_s(str,STRLEN,"%02d [%02d:%02d:%06.3f] %s\n",n + 1,(int)h,(int)m,s,m_strTitle[n]);
		SendDlgItemMessage(m_hDlg,IDC_LIST1,LB_ADDSTRING,0L,(LPARAM)str);
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
	}
	m_numChapter++;

	strcpy_s(m_strTitle[ins],STRLEN,str);
	m_Frame[ins] = m_frame;
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
	}
	ShowList();
}

void CfgDlg::Seek() {
	LRESULT sel;

	sel = SendDlgItemMessage(m_hDlg,IDC_LIST1,LB_GETCURSEL,0,0);
	if(sel == LB_ERR) return;

	if(m_Frame[sel] == m_frame) return;
	m_exfunc->set_frame(m_editp,m_Frame[sel]);
	SetDlgItemText(m_hDlg,IDC_EDNAME,m_strTitle[sel]);
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
	sprintf_s(m_strTime,STRLEN,"%02d:%02d:%06.3f",(int)h,(int)m,s);
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
	LONGLONG t;
	int h,m,s;
	int frame;
	char str[STRLEN],path[_MAX_PATH];
	FILE *file;
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
	if(GetOpenFileName(&of) == 0) return;

	if(fopen_s(&file,path,"r")) {
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
		m_numChapter++;
		if(m_numChapter > 100) break;
	}
	fclose(file);
	ShowList();
}