//----------------------------------------------------------------------------------
//		チャプター編集プラグイン by ぽむ 
//----------------------------------------------------------------------------------
#include <windows.h>
#include <imm.h>
#include "config.h"
#include "resource.h"
#include "mylib.h"

//---------------------------------------------------------------------
//		フィルタ構造体定義
//---------------------------------------------------------------------
FILTER_DLL filter = {
	FILTER_FLAG_ALWAYS_ACTIVE|FILTER_FLAG_MAIN_MESSAGE|FILTER_FLAG_WINDOW_SIZE|FILTER_FLAG_DISP_FILTER|FILTER_FLAG_EX_INFORMATION,	// int flag
	567,435,	// int x,y
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
	"チャプター編集 ver0.6 by ぽむ + 無音＆シーンチェンジ検索機能 by ru",	// TCHAR *information
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
	if (!g_keyhook && nCode == HC_ACTION && wParam == 0x0D && GetForegroundWindow() == g_hwnd) {
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

BOOL func_WndProc(HWND hwnd,UINT message,WPARAM wparam,LPARAM lparam,void *editp,FILTER *fp)
{
	FILE_INFO fip;

	switch(message) {
		case WM_FILTER_INIT:
			g_config.Init(hwnd,editp,fp);
			g_hwnd = hwnd;
			g_hHook = SetWindowsHookEx(WH_KEYBOARD,KeyboardProc,0,GetCurrentThreadId());
			g_keyhook = 0;
			break;
		case WM_FILTER_EXIT:
			UnhookWindowsHookEx(g_hHook);
			break;
		case WM_FILTER_UPDATE:	//編集操作
			g_config.SetFrame(fp->exfunc->get_frame(editp));
			g_config.SetFrameN(editp,fp->exfunc->get_frame_n(editp));
			break;
		case WM_FILTER_FILE_OPEN:	//ファイル読み込み
			if(fp->exfunc->get_file_info(editp,&fip)) g_config.SetFps(fip.video_rate,fip.video_scale);
			g_config.SetFrameN(editp,fp->exfunc->get_frame_n(editp));
			break;
		//[ru]閉じたとき
		case WM_FILTER_FILE_CLOSE:
			g_config.SetFrameN(NULL, 0);
			g_config.SetFps(10, 10);
			break;
		//ここまで
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
					g_config.AuotSaveCheck();
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

	g_prf.m_numChapter = g_config.m_numChapter;
	CopyMemory(g_prf.m_Frame,g_config.m_Frame,sizeof(int)*100);
	CopyMemory(g_prf.m_strTitle,g_config.m_strTitle,sizeof(char)*100*STRLEN);
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
		g_config.ShowList();
	}
	return TRUE;
}