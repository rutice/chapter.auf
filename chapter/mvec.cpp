// IIR_3DNRフィルタ  by H_Kasahara(aka.HK) より拝借


//---------------------------------------------------------------------
//		動き検索処理用
//---------------------------------------------------------------------

#include <Windows.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>

#define FRAME_PICTURE	1
#define FIELD_PICTURE	2
#define MAX_SEARCH_EXTENT 32	//全探索の最大探索範囲。+-この値まで。

//---------------------------------------------------------------------
//		関数定義
//---------------------------------------------------------------------
void make_motion_lookup_table();
BOOL mvec(unsigned char* current_pix,unsigned char* bef_pix,int* vx,int* vy,int lx,int ly,int threshold,int pict_struct,int SC_level);
int tree_search(unsigned char* current_pix,unsigned char* bef_pix,int lx,int ly,int *vx,int *vy,int search_block_x,int search_block_y,int min,int pict_struct);
int full_search(unsigned char* current_pix,unsigned char* bef_pix,int lx,int ly,int *vx,int *vy,int search_block_x,int search_block_y,int min,int pict_struct, int search_extent);
int dist( unsigned char *p1, unsigned char *p2, int lx, int distlim, int block_hight );

//---------------------------------------------------------------------
//		グローバル変数
//---------------------------------------------------------------------
int	block_hight, lx2;

int x_plus[] = {1,1,1,2,1,1,1,1,
				1,1,1,2,1,1,1,1,
				1,1,1,2,1,1,1,1,
				1,1,1,2,1,1,1,1,
				1,1,1,2,1,1,1,1,};
int dxplus[] = {16,16,16,32,16,16,16, 0,
				 8, 8, 8,16, 8, 8, 8, 0,
				 4, 4, 4, 8, 4, 4, 4, 0,
				 2, 2, 2, 4, 2, 2, 2, 0,
				 1, 1, 1, 2, 1, 1, 1, 0,};
//---------------------------------------------------------------------
//		動き誤差判定関数
//---------------------------------------------------------------------
//[ru] 動きベクトルの合計を返す
int tree=0, full=0;
int mvec(unsigned char* current_pix, 	//現フレームの輝度。8ビット。
		  unsigned char* bef_pix,		//前フレームの輝度。8ビット。
		  int lx,						//画像の横幅
		  int ly,						//画像の縦幅
		  int threshold,				//検索精度。(100-fp->track[1])*50 …… 50は適当な値。
		  int pict_struct)				//"1"ならフレーム処理、"2"ならフィールド処理
{
	int x, y;
	unsigned char *p1, *p2;
	int motion_vector_total = 0;

//関数を呼び出す毎に計算せずにすむようグローバル変数とする
	lx2 = lx*pict_struct;
	block_hight = 16/pict_struct;

	for(int i=0;i<pict_struct;i++)
	{
		for(y=i;y<ly;y+=16)	//全体縦軸
		{
			p1 = current_pix + y*lx;
			p2 = bef_pix + y*lx;
			for(x=0;x<lx;x+=16)	//全体横軸
			{
				int vx=0, vy=0;
				int min = dist( p1, p2, lx2, INT_MAX, block_hight );
				if( threshold < (min = tree_search( p1, p2, lx, ly, &vx, &vy, x, y, min, pict_struct)) && (vx!=0 || vy!=0) )
					if( threshold < (min = tree_search( p1, &p2[vy * lx + vx], lx, ly, &vx, &vy, x+vx, y+vy, min, pict_struct)) )
						if( threshold < (min = tree_search( p1, &p2[vy * lx + vx], lx, ly, &vx, &vy, x+vx, y+vy, min, pict_struct)) )
//3回のツリー探索でもフレーム間の絶対値差が大きければ全探索をおこなう
							full_search( p1, &p2[vy * lx + vx], lx, ly, &vx, &vy, x+vx, y+vy, min, pict_struct, max(abs(vx),abs(vy))*2 );

//動きベクトルの合計がシーンチェンジレベルを超えていたら、シーンチェンジと判定してTRUEを返して終了
				motion_vector_total += abs(vx)+abs(vy);

				p1+=16;
				p2+=16;
			}
		}
	}

	/*char str[500];
	sprintf_s(str, 500, "tree:%d, full:%d", tree, full);
	MessageBox(NULL, str, 0, 0);*/

	return motion_vector_total;
}
//---------------------------------------------------------------------
//		ツリー探索法動き検索関数
//---------------------------------------------------------------------
int tree_search(unsigned char* current_pix,	//現フレームの輝度。8ビット。
				unsigned char* bef_pix,		//前フレームの輝度。8ビット。
				int lx,						//画像の横幅
				int ly,						//画像の縦幅
				int *vx,					//x方向の動きベクトルが代入される。
				int *vy,					//y方向の動きベクトルが代入される。
				int search_block_x,			//検索位置
				int search_block_y,			//検索位置
				int min,					//同位置でのフレーム間の絶対値差。関数内では同位置の比較をしないので、呼び出す前に行う必要あり。
				int pict_struct)			//"1"ならフレーム処理、"2"ならフィールド処理
{
	tree++;
	int dx, dy, ddx=0, ddy=0, xs=0, ys;
	int d;
	int x,y;
	int speedup = pict_struct-1;
	int *x_plus_p = x_plus,
		*dxplus_p = dxplus;
//検索範囲の上限と下限を設定
	int ylow  = 0 - search_block_y;
	int yhigh = ly- search_block_y-16;
	int xlow  = 0 - search_block_x;
	int xhigh = lx- search_block_x-16;

//検索範囲が画像からはみ出ないようなら、処理高速化のために、画面外に出たかのチェックを行わないルーチンを使用する。
	if(-31<ylow || yhigh<31 || -31<xlow || xhigh<31)
	{
		for(int i=16;i>speedup;i=i/2){	//検索回数
//前フレームのブロックの位置を移動させ現フレームのとの絶対値差を追加していく。
			for(y=0,dy=ddy-i;y<3;y++,dy+=i){
				if( dy<ylow || dy>yhigh )	continue;	//検索位置が画面外に出ていたら検索をおこなわない。
				ys = dy * lx;	//検索位置縦軸
				for(x=0,dx=xs-i;x<3;x+=*x_plus_p,dx+=*dxplus_p){
					if( dx<xlow || dx>xhigh )	continue;	//検索位置が画面外に出ていたら検索をおこなわない。
					d = dist( current_pix, &bef_pix[ys+dx], lx2, min, block_hight );
					if( d <= min ){	//これまでの検索よりフレーム間の絶対値差が小さかったらそれぞれ代入。
						min = d;
						ddx = dx;
						ddy = dy;
					}
					x_plus_p++;
					dxplus_p++;
				}
			}
			xs = ddx;
		}
		if(pict_struct==FIELD_PICTURE){
			for(x=0,dx=ddx-1;x<3;x+=2,dx+=2){
				if( search_block_x+dx<0 || search_block_x+dx+16>lx )	continue;	//検索位置が画面外に出ていたら検索をおこなわない。
				d = dist( current_pix, &bef_pix[ys+dx], lx2, min, block_hight );
				if( d <= min ){	//これまでの検索よりフレーム間の絶対値差が小さかったらそれぞれ代入。
					min = d;
					ddx = dx;
				}
			}
		}
	}
	else
	{
		for(int i=16;i>speedup;i=i/2){	//検索回数
//前フレームのブロックの位置を移動させ現フレームのとの絶対値差を追加していく。
			for(y=0,dy=ddy-i;y<3;y++,dy+=i){
				ys = dy * lx;	//検索位置縦軸
				for(x=0,dx=xs-i;x<3;x+=*x_plus_p,dx+=*dxplus_p){
					d = dist( current_pix, &bef_pix[ys+dx], lx2, min, block_hight );
					if( d <= min ){	//これまでの検索よりフレーム間の絶対値差が小さかったらそれぞれ代入。
						min = d;
						ddx = dx;
						ddy = dy;
					}
					x_plus_p++;
					dxplus_p++;
				}
			}
			xs = ddx;
		}
		if(pict_struct==FIELD_PICTURE){
			for(x=0,dx=ddx-1;x<3;x+=2,dx+=2){
				d = dist( current_pix, &bef_pix[ys+dx], lx2, min, block_hight );
				if( d <= min ){	//これまでの検索よりフレーム間の絶対値差が小さかったらそれぞれ代入。
					min = d;
					ddx = dx;
				}
			}
		}
	}

	*vx += ddx;
	*vy += ddy;

	return min;
}
//---------------------------------------------------------------------
//		全探索法動き検索関数
//---------------------------------------------------------------------
int full_search(unsigned char* current_pix,	//現フレームの輝度。8ビット。
				unsigned char* bef_pix,		//前フレームの輝度。8ビット。
				int lx,						//画像の横幅
				int ly,						//画像の縦幅
				int *vx,					//x方向の動きベクトルが代入される。
				int *vy,					//y方向の動きベクトルが代入される。
				int search_block_x,			//検索位置
				int search_block_y,			//検索位置
				int min,					//フレーム間の絶対値差。最初の探索ではINT_MAXが入っている。
				int pict_struct,			//"1"ならフレーム処理、"2"ならフィールド処理
				int search_extent)			//探索範囲。
{
	full++;
	int dx, dy, ddx=0, ddy=0, ys;
	int d;
	int search_point;
	unsigned char* p2;

	if(search_extent>MAX_SEARCH_EXTENT)
		search_extent = MAX_SEARCH_EXTENT;

//検索範囲の上限と下限が画像からはみ出していないかチェック
	int ylow  = 0 - ( (search_block_y-search_extent<0) ? search_block_y : search_extent );
	int yhigh = (search_block_y+search_extent+16>ly) ? ly-search_block_y-16 : search_extent;
	int xlow  = 0 - ( (search_block_x-search_extent<0) ? search_block_x : search_extent );
	int xhigh = (search_block_x+search_extent+16>lx) ? lx-search_block_x-16 : search_extent;

	for(dy=ylow;dy<=yhigh;dy+=pict_struct)
	{
		p2 = bef_pix + dy*lx + xlow;	//Y軸検索位置。xlowは負の値なので"p2=bef_pix+dy*lx-xlow"とはならない
		for(dx=xlow;dx<=xhigh;dx++)
		{
			d = dist( current_pix, p2, lx2, min, block_hight );
			if(d <= min)	//これまでの検索よりフレーム間の絶対値差が小さかったらそれぞれ代入。
			{
				min = d;
				ddx = dx;
				ddy = dy;
			}
			p2++;
		}
	}

	*vx += ddx;
	*vy += ddy;

	return min;
}
//---------------------------------------------------------------------
//		フレーム間絶対値差合計関数
//---------------------------------------------------------------------
//bbMPEGのソースを流用
#include <emmintrin.h>

int dist( unsigned char *p1, unsigned char *p2, int lx, int distlim, int block_height )
{
	if (block_height == 8) {
		__m128i a, b, r;

		a = _mm_load_si128 ((__m128i*)p1 +  0);
		b = _mm_loadu_si128((__m128i*)p2 +  0);
		r = _mm_sad_epu8(a, b);
		
		a = _mm_load_si128 ((__m128i*)(p1 + lx));
		b = _mm_loadu_si128((__m128i*)(p2 + lx));
		r = _mm_add_epi32(r, _mm_sad_epu8(a, b));
		
		a = _mm_load_si128 ((__m128i*)(p1 + 2*lx));
		b = _mm_loadu_si128((__m128i*)(p2 + 2*lx));
		r = _mm_add_epi32(r, _mm_sad_epu8(a, b));
		
		a = _mm_load_si128 ((__m128i*)(p1 + 3*lx));
		b = _mm_loadu_si128((__m128i*)(p2 + 3*lx));
		r = _mm_add_epi32(r, _mm_sad_epu8(a, b));
		
		a = _mm_load_si128 ((__m128i*)(p1 + 4*lx));
		b = _mm_loadu_si128((__m128i*)(p2 + 4*lx));
		r = _mm_add_epi32(r, _mm_sad_epu8(a, b));
		
		a = _mm_load_si128 ((__m128i*)(p1 + 5*lx));
		b = _mm_loadu_si128((__m128i*)(p2 + 5*lx));
		r = _mm_add_epi32(r, _mm_sad_epu8(a, b));
		
		a = _mm_load_si128 ((__m128i*)(p1 + 6*lx));
		b = _mm_loadu_si128((__m128i*)(p2 + 6*lx));
		r = _mm_add_epi32(r, _mm_sad_epu8(a, b));
		
		a = _mm_load_si128 ((__m128i*)(p1 + 7*lx));
		b = _mm_loadu_si128((__m128i*)(p2 + 7*lx));
		r = _mm_add_epi32(r, _mm_sad_epu8(a, b));
		return _mm_extract_epi16(r, 0) + _mm_extract_epi16(r, 4);;
	}
	
	int s = 0;
	for(int i=0;i<block_height;i++)
	{
		/*
		s += motion_lookup[p1[0]][p2[0]];
		s += motion_lookup[p1[1]][p2[1]];
		s += motion_lookup[p1[2]][p2[2]];
		s += motion_lookup[p1[3]][p2[3]];
		s += motion_lookup[p1[4]][p2[4]];
		s += motion_lookup[p1[5]][p2[5]];
		s += motion_lookup[p1[6]][p2[6]];
		s += motion_lookup[p1[7]][p2[7]];
		s += motion_lookup[p1[8]][p2[8]];
		s += motion_lookup[p1[9]][p2[9]];
		s += motion_lookup[p1[10]][p2[10]];
		s += motion_lookup[p1[11]][p2[11]];
		s += motion_lookup[p1[12]][p2[12]];
		s += motion_lookup[p1[13]][p2[13]];
		s += motion_lookup[p1[14]][p2[14]];
		s += motion_lookup[p1[15]][p2[15]];*/
		
		__m128i a = _mm_load_si128((__m128i*)p1);
		__m128i b = _mm_loadu_si128((__m128i*)p2);
		__m128i r = _mm_sad_epu8(a, b);
		s += _mm_extract_epi16(r, 0) + _mm_extract_epi16(r, 4);

		if (s > distlim)	break;

		p1 += lx;
		p2 += lx;
	}
	return s;
}
//---------------------------------------------------------------------
//		フレーム間絶対値差合計関数(SSEバージョン)
//---------------------------------------------------------------------
int dist_SSE( unsigned char *p1, unsigned char *p2, int lx, int distlim, int block_hight )
{
	int s = 0;
/*
dist_normalを見ると分かるように、p1とp2の絶対値差を足してき、distlimを超えたらその合計を返すだけ。
block_hightには8か16が代入されており、前者はフィールド処理、後者がフレーム処理用。
block_hightに8が代入されていたらば、lxには画像の横幅が代入されている。
block_hightに16が代入されていたらば、lxには画像の横幅の二倍の値が代入されている。
どなたか、ここを作成していただけたらば、非常に感謝いたします。
*/
	return s;
}