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

int my_getbuild(FILTER *fp,void *editp);		// AviUtlのBuild番号の取得(0.99e2以降)
HFONT my_getfont(FILTER *fp,void *editp);		// AviUtlのフォントの取得
int my_numthreads(FILTER *fp);					// AviUtlのスレッド数を取得
bool my_sjis(char *chr,int pos);				// SJISの1文字目判定
void my_getpath(char *path,int length);			// パス名の抽出（ファイル名の前まで）
void my_getexepath(char *path,int length);		// AviUtlのあるパス名の取得
void my_getaufpath(FILTER *fp,char *path,int length);	// プラグインのあるパス名の取得
