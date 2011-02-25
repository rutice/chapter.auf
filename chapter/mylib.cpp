//---------------------------------------------------------------------
//		自分用の適当なライブラリ by ぽむ
//---------------------------------------------------------------------
#include <windows.h>
#include "filter.h"
#include "mylib.h"

// AviUtlのBuild番号の取得(0.99e2以降)
int my_getbuild(FILTER *fp,void *editp) {
	SYS_INFO sip;

	fp->exfunc->get_sys_info(editp,&sip);
	return sip.build < VER_99e2 ? 0 : sip.build;
}

// AviUtlのフォントの取得
HFONT my_getfont(FILTER *fp,void *editp) {
	SYS_INFO sip;

	fp->exfunc->get_sys_info(editp,&sip);
	return sip.hfont ? sip.hfont : (HFONT)GetStockObject(DEFAULT_GUI_FONT);
}

// AviUtlのスレッド数を取得
void my_multi_thread_func(int thread_id,int thread_num,void *param1,void *param2){
	int *threads = (int *) param1;
	*threads = thread_num;
}
int my_numthreads(FILTER *fp) {
	int num_threads;

	if(!fp->exfunc->exec_multi_thread_func) return 1;
	fp->exfunc->exec_multi_thread_func(my_multi_thread_func,(void*)&num_threads,NULL);
	return num_threads;
}

// SJISの1文字目判定
bool my_sjis(char *chr,int pos) {
	PUCHAR chr2 = (PUCHAR)chr;
	UCHAR c;

	if(pos < 0) return false;
	c = chr2[pos];
	if((c >= 0x81 && c <= 0x9f)||(c >= 0xe0 && c <= 0xfc)) return true;
	return false;
}

// パス名の抽出（ファイル名の前まで）
void my_getpath(char *path,int length) {
	int pos = 0;

	for(int i = 0; i < length; i++) {
		if(path[i] == NULL) break;
		if(path[i] == '\\' && !my_sjis(path,i-1)) pos = i;
	}

	path[pos+1] = NULL;
}

// AviUtlのあるパス名の取得
void my_getexepath(char *path,int length) {
	GetModuleFileName(NULL,path,length);
	my_getpath(path,length);
}

// プラグインのあるパス名の取得
void my_getaufpath(FILTER *fp,char *path,int length) {
	GetModuleFileName(fp->dll_hinst,path,length);
	my_getpath(path,length);
}
