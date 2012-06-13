//----------------------------------------------------------------------------------
//		チャプター編集プラグイン by ぽむ 
//----------------------------------------------------------------------------------
#include <windows.h>
#include <Shlwapi.h>
#include <imm.h>
#include "config.h"
#include "resource.h"
#include "mylib.h"

#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "imm32.lib")

const int MIN_WIDTH = 550;
const int MIN_HEIGHT = 435;

//---------------------------------------------------------------------
//		フィルタ構造体定義
//---------------------------------------------------------------------
FILTER_DLL filter = {
	FILTER_FLAG_ALWAYS_ACTIVE|FILTER_FLAG_MAIN_MESSAGE|FILTER_FLAG_WINDOW_THICKFRAME|FILTER_FLAG_WINDOW_SIZE|FILTER_FLAG_DISP_FILTER|FILTER_FLAG_EX_INFORMATION,	// int flag
	MIN_WIDTH, MIN_HEIGHT,	// int x,y
	"チャプター編集",	// TCHAR *name
	NULL,NULL,NULL,	// int track_n, TCHAR **track_name, int *track_default
	NULL,NULL,	// int *track_s, *track_e
	NULL,NULL,NULL,	// int check_n, TCHAR **check_name, int *check_default
	NULL,	// (*func_proc)
	NULL,	// (*func_init)
	NULL,	// (*func_exit)
	NULL,	// (*func_update)
	func_WndProc,	// (*func_WndProc)
	NULL,NULL,	// reserved
	NULL,	// void *ex_data_ptr
	NULL,	// int ex_data_size
	"チャプター編集 ver0.6 by ぽむ + 無音＆シーンチェンジ検索機能 by ru + 削除追従 by fe",	// TCHAR *information
	func_save_start,	// (*func_save_start)
	NULL,	// (*func_save_end)
	NULL,	// EXFUNC *exfunc;
	NULL,	// HWND	hwnd;
	NULL,	// HINSTANCE dll_hinst;
	NULL,	// void	*ex_data_def;
	NULL,	// (*func_is_saveframe)
	func_project_load,	// (*func_project_load)
	func_project_save,	// (*func_project_save)
	NULL,	// (*func_modify_title)
	NULL,	// TCHAR *dll_path;
};

//---------------------------------------------------------------------
//		変数
//---------------------------------------------------------------------
CfgDlg	g_config;
PrfDat	g_prf;
HHOOK	g_hHook;
HHOOK	g_hMessageHook;
HWND	g_hwnd;
int		g_keyhook;

//---------------------------------------------------------------------
//		フィルタ構造体のポインタを渡す関数
//---------------------------------------------------------------------
EXTERN_C FILTER_DLL __declspec(dllexport) * __stdcall GetFilterTable( void )
{
	return &filter;
}

//---------------------------------------------------------------------
//		YUY2フィルタ構造体のポインタを渡す関数
//---------------------------------------------------------------------
EXTERN_C FILTER_DLL __declspec(dllexport) * __stdcall GetFilterTableYUY2( void )
{
	return &filter;
}

//---------------------------------------------------------------------
//		func_save_start
//---------------------------------------------------------------------
BOOL func_save_start(FILTER *fp,int s,int e,void *editp) {
	g_config.AutoSave();

	return TRUE;
}

//---------------------------------------------------------------------
//		func_WndProc
//---------------------------------------------------------------------
LRESULT CALLBACK KeyboardProc(int nCode,WPARAM wParam,LPARAM lParam)
{
	static int ime = 0;

	// Enterキーで入力
	if (!g_keyhook && nCode == HC_ACTION && wParam == 0x0D && ((lParam & (1 << 30)) == 0) && GetForegroundWindow() == g_hwnd) {
		// IMEでのEnterを無視する
		HIMC hIMC = ImmGetContext(g_hwnd);
		if(ImmGetOpenStatus(hIMC) && ImmGetCompositionString(hIMC, GCS_COMPSTR, NULL, 0)) ime = 2;
		ImmReleaseContext(g_hwnd,hIMC);
		if(!ime) {
			g_config.AddList();
			return TRUE;
		}
	}
	if(ime) ime--;
	if(g_keyhook) g_keyhook--;	// ダイアログでのEnterを無視するのに使う

	return CallNextHookEx(g_hHook,nCode,wParam,lParam);
}

LRESULT CALLBACK WindowMessageProc(int nCode, WPARAM wp, LPARAM lp)
{
	if(nCode == HC_ACTION){
		MSG* msg = (MSG*)lp;
		//フレーム削除が呼ばれたら処理
		if(wp == PM_REMOVE && msg->message == WM_COMMAND && LOWORD(msg->wParam) == 0x13ED){
			g_config.UpdateFramePos();
		}
	}

	return CallNextHookEx(g_hMessageHook, nCode, wp, lp);
}

BOOL SearchFile(const char *lpcSearch, char *lpOut, int cch) {
	WIN32_FIND_DATA ffd;
	HANDLE hFind = FindFirstFile(lpcSearch, &ffd);
	if (hFind == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
	while ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
		if (!FindNextFile(hFind, &ffd)) {
			break;
		}
	}
	BOOL bRet = FALSE;
	if (GetLastError() != ERROR_NO_MORE_FILES) {
		strcpy_s(lpOut, cch, ffd.cFileName);
		bRet = TRUE;
	}
	FindClose(hFind);
	return bRet;
}

BOOL OnDropFiles(WPARAM wparam, LPARAM lparam, void *editp, FILTER *fp) {
	HDROP hDrop = (HDROP)wparam;
	int nFiles = DragQueryFile(hDrop, -1, NULL, 0);
	char szFile[2000];
	DragQueryFile(hDrop, 0, szFile, 1000);
	char *pExt = PathFindExtension(szFile);
	if (_stricmp(pExt, ".txt") == 0) {
		// chapterファイル
		g_config.LoadFromFile(szFile);
	} else if (nFiles > 1) {
		// 1つめが映像、2つめが音声と期待
		fp->exfunc->edit_open(editp, szFile, 0);
		DragQueryFile(hDrop, 1, szFile, 1000);
		fp->exfunc->edit_open(editp, szFile, EDIT_OPEN_FLAG_AUDIO);
	} else if (_stricmp(pExt, ".wav") == 0 || _stricmp(pExt, ".aac") == 0) {
		// wav, aacファイル(読める入力プラグインがあることを期待)
		fp->exfunc->edit_open(editp, szFile, EDIT_OPEN_FLAG_AUDIO);
	} else if (_stricmp(pExt, ".ts") == 0 || _stricmp(pExt, ".m2v") == 0) {
		// ts, m2vファイル (別に音声ファイルがあれば読み込む)
		fp->exfunc->edit_open(editp, szFile, 0);
		strcpy_s(pExt, 100, "*.wav");
		if (SearchFile(szFile, PathFindFileName(szFile), 1000)) {
				fp->exfunc->edit_open(editp, szFile, EDIT_OPEN_FLAG_AUDIO);
		} else {
			strcpy_s(pExt, 100, "*.aac");
			if (SearchFile(szFile, PathFindFileName(szFile), 1000)) {
				fp->exfunc->edit_open(editp, szFile, EDIT_OPEN_FLAG_AUDIO);
			}
		}
	}
	DragFinish(hDrop);
	return TRUE;
}

BOOL func_WndProc(HWND hwnd,UINT message,WPARAM wparam,LPARAM lparam,void *editp,FILTER *fp)
{
	FILE_INFO fip;

	switch(message) {
		case WM_FILTER_INIT:
			g_config.Init(hwnd,editp,fp);
			
			fp->exfunc->add_menu_item(fp, "前のチャプター", hwnd, IDM_CHAP_PREV, 0, 0);
			fp->exfunc->add_menu_item(fp, "次のチャプター", hwnd, IDM_CHAP_NEXT, 0, 0);
			fp->exfunc->add_menu_item(fp, "現在のフレームの前のチャプター", hwnd, IDM_CHAP_PREVHERE, VK_OEM_PERIOD, 0);
			fp->exfunc->add_menu_item(fp, "現在のフレームの後のチャプター", hwnd, IDM_CHAP_NEXTHERE, VK_OEM_2, 0);
			fp->exfunc->add_menu_item(fp, "チャプターを削除", hwnd, IDM_CHAP_DELETE, 0, 0);

			// ドラッグ＆ドロップ対応
			DragAcceptFiles(hwnd, TRUE);

			g_hwnd = hwnd;
			g_hHook = SetWindowsHookEx(WH_KEYBOARD,KeyboardProc,0,GetCurrentThreadId());
			g_keyhook = 0;
			g_hMessageHook = SetWindowsHookEx(WH_GETMESSAGE,WindowMessageProc,0,GetCurrentThreadId());
			break;
		case WM_FILTER_EXIT:
			// ドラッグ＆ドロップ終了
			DragAcceptFiles(hwnd, FALSE);

			UnhookWindowsHookEx(g_hMessageHook);
			UnhookWindowsHookEx(g_hHook);
			break;
		case WM_FILTER_UPDATE:	//編集操作
			g_config.SetFrame(fp->exfunc->get_frame(editp));
			g_config.SetFrameN(editp,fp->exfunc->get_frame_n(editp));
			break;
		case WM_FILTER_FILE_OPEN:	//ファイル読み込み
			if(fp->exfunc->get_file_info(editp,&fip)) g_config.SetFps(fip.video_rate,fip.video_scale);
			g_config.SetFrameN(editp,fp->exfunc->get_frame_n(editp));
			// 同名のtxtファイルがあれば、chapterファイルとして読み込む
			char path[MAX_PATH];
			strcpy_s(path, fip.name);
			PathRenameExtension(path, ".txt");
			if (PathFileExists(path)) {
				g_config.LoadFromFile(path);
			}
			break;
		//[ru]閉じたとき
		case WM_FILTER_FILE_CLOSE:
			g_config.SetFrameN(NULL, 0);
			g_config.SetFps(10, 10);
			break;
		case WM_FILTER_COMMAND:
			switch(wparam) {
			case IDM_CHAP_NEXT:
				g_config.NextList();
				return TRUE;
			case IDM_CHAP_PREV:
				g_config.PrevList();
				return TRUE;
			case IDM_CHAP_NEXTHERE:
				g_config.NextHereList();
				return TRUE;
			case IDM_CHAP_PREVHERE:
				g_config.PrevHereList();
				return TRUE;
			case IDM_CHAP_DELETE:
				g_config.DelList();
				return TRUE;
			}
			break;
		// DnD
		case WM_DROPFILES:
			return OnDropFiles(wparam, lparam, editp, fp);

		case WM_GETMINMAXINFO:
			((MINMAXINFO*)lparam)->ptMinTrackSize.x = MIN_WIDTH;
			((MINMAXINFO*)lparam)->ptMinTrackSize.y = MIN_HEIGHT;
			break;

		case WM_SIZE:
			g_config.Resize();
			break;

		case WM_COMMAND:
			switch(LOWORD(wparam)) {
				case IDC_BUADD:
					g_config.AddList();
					break;
				case IDC_BUDEL:
					g_config.DelList();
					break;
				case IDC_LIST1:
					if(HIWORD(wparam) != LBN_SELCHANGE) break;
					g_config.Seek();
					return TRUE;
				case IDC_BUSAVE:
					g_config.Save();
					g_keyhook = 1;
					break;
				case IDC_BULOAD:
					g_config.Load();
					g_keyhook = 1;
					break;
				case IDC_CHECK1:
				//[ru]ついでに
				case IDC_CHECKSC:
				//ここまで
				case IDC_SCMARK:
				case IDC_PRECHECK:
					g_config.AutoSaveCheck();
					break;
				//[ru]追加
				case IDC_BUDETECT:
					g_config.DetectMute();
					break;
				//ここまで
			}
			break;
	}
	return FALSE;
}

//---------------------------------------------------------------------
//		func_project_save
//---------------------------------------------------------------------
BOOL func_project_save( FILTER *fp,void *editp,void *data,int *size ) {
	*size = sizeof(PrfDat);
	if(data == NULL) return TRUE;	// この関数は2回呼ばれる

	g_prf.m_numChapter = min(100, g_config.m_numChapter);
	CopyMemory(g_prf.m_Frame,g_config.m_Frame,sizeof(int)*100);
	CopyMemory(g_prf.m_strTitle,g_config.m_strTitle,sizeof(char)*100*STRLEN);
	CopyMemory(g_prf.m_SCPos,g_config.m_SCPos,sizeof(int)*100);
	CopyMemory(data,&g_prf,*size);
	return TRUE;
}

//---------------------------------------------------------------------
//		func_project_load
//---------------------------------------------------------------------
BOOL func_project_load( FILTER *fp,void *editp,void *data,int size ) {
	if(size == sizeof(PrfDat)) {
		CopyMemory(&g_prf,data,size);
		g_config.m_numChapter = g_prf.m_numChapter;
		CopyMemory(g_config.m_Frame,g_prf.m_Frame,sizeof(int)*100);
		CopyMemory(g_config.m_strTitle,g_prf.m_strTitle,sizeof(char)*100*STRLEN);
		CopyMemory(g_config.m_SCPos,g_prf.m_SCPos,sizeof(int)*100);
		g_config.ShowList();
	}
	return TRUE;
}
