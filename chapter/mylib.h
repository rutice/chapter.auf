//---------------------------------------------------------------------
//		自分用の適当なライブラリ by ぽむ
//---------------------------------------------------------------------

// AviUtlのBuild番号
#define VER_99e2	9912
#define VER_99f		9913
#define VER_99g3	9916
#define VER_99g4	9917

// フルスケールでの色空間変換式
// 輝度(Y)範囲：0～4096
// 色差青(Cb)範囲：-2048～2048
// 色差赤(Cr)範囲：-2048～2048
// 赤色(R)範囲：0～4096
// 緑色(G)範囲：0～4096
// 青色(B)範囲：0～4096
#define YC2R(Y,Cb,Cr) (Y + 1.402*Cr)
#define YC2G(Y,Cb,Cr) (Y - 0.344*Cb - 0.714*Cr)
#define YC2B(Y,Cb,Cr) (Y + 1.772*Cb)
#define RGB2Y(R,G,B) (0.299*R + 0.587*G + 0.114*B)
#define RGB2Cb(R,G,B) (-0.169*R - 0.331*G + 0.500*B)
#define RGB2Cr(R,G,B) (0.500*R - 0.419*G - 0.081*B)

// YCbCrからRGBに変換
inline void my_yc2rgb(int Y,int Cb,int Cr,int *R,int *G,int *B) {
	*R = (int)YC2R(Y,Cb,Cr);
	*G = (int)YC2G(Y,Cb,Cr);
	*B = (int)YC2B(Y,Cb,Cr);
}
inline void my_yc2rgb(int Y,int Cb,int Cr,short *R,short *G,short *B) {
	*R = (short)YC2R(Y,Cb,Cr);
	*G = (short)YC2G(Y,Cb,Cr);
	*B = (short)YC2B(Y,Cb,Cr);
}

// RGBからYCbCrに変換
inline void my_rgb2yc(int R,int G,int B,int *Y,int *Cb,int *Cr) {
	*Y = (int)RGB2Y(R,G,B);
	*Cb = (int)RGB2Cb(R,G,B);
	*Cr = (int)RGB2Cr(R,G,B);
}
inline void my_rgb2yc(int R,int G,int B,short *Y,short *Cb,short *Cr) {
	*Y = (short)RGB2Y(R,G,B);
	*Cb = (short)RGB2Cb(R,G,B);
	*Cr = (short)RGB2Cr(R,G,B);
}

// 時間とフレーム番号の変換
inline
int time2frame(int h, int m, int s, int ms, int rate, int scale) {
	s = s * 1000 + ms;
	LONGLONG t = (LONGLONG)h * 3600000 + (LONGLONG)m * 60000 + (LONGLONG)s;

	// 端数は四捨五入
	int frame = (int)(t * rate / scale / (1000 / 10) + 5) / 10;
	if (frame < 0) {
		frame = 0;
	}
	return frame;
}

inline
std::string frame2time(int frame, int rate, int scale) {
	LONGLONG t = (LONGLONG)frame * 10000000 * scale / rate;
	LONGLONG h = (t / 36000000000);
	LONGLONG m = ((t - h * 36000000000) / 600000000);
	double s = (t - h * 36000000000 - m * 600000000) / 10000000.0;

	char ret[50];
	sprintf_s(ret, "%02d:%02d:%06.3f", (int)h, (int)m, s);
	return std::string(ret);
}

// AviUtlのBuild番号の取得(0.99e2以降)
inline
int my_getbuild(FILTER *fp,void *editp) {
	SYS_INFO sip;

	fp->exfunc->get_sys_info(editp,&sip);
	return sip.build < VER_99e2 ? 0 : sip.build;
}
// AviUtlのフォントの取得
inline
HFONT my_getfont(FILTER *fp,void *editp) {
	SYS_INFO sip;

	fp->exfunc->get_sys_info(editp,&sip);
	return sip.hfont ? sip.hfont : (HFONT)GetStockObject(DEFAULT_GUI_FONT);
}

// AviUtlのスレッド数を取得
inline
void my_multi_thread_func(int thread_id,int thread_num,void *param1,void *param2){
	int *threads = (int *) param1;
	*threads = thread_num;
}
inline
int my_numthreads(FILTER *fp) {
	int num_threads;

	if(!fp->exfunc->exec_multi_thread_func) return 1;
	fp->exfunc->exec_multi_thread_func(my_multi_thread_func,(void*)&num_threads,NULL);
	return num_threads;
}

// パス名の抽出（ファイル名の前まで）
inline
void my_getpath(char *path,int length) {
	char *p = PathFindFileName(path);
	if (p) {
		*p = '\0';
	}
}

// AviUtlのあるパス名の取得
inline
void my_getexepath(char *path,int length) {
	GetModuleFileName(NULL,path,length);
	my_getpath(path,length);
}

// プラグインのあるパス名の取得
inline
void my_getaufpath(FILTER *fp,char *path,int length) {
	GetModuleFileName(fp->dll_hinst,path,length);
	my_getpath(path,length);
}
