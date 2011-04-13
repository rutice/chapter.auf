// chapter_exe.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include "source.h"
#include "faw.h"

// mvec.c
#define FRAME_PICTURE	1
#define FIELD_PICTURE	2
int mvec(unsigned char* current_pix,unsigned char* bef_pix,int lx,int ly,int threshold,int pict_struct);

void write_chapter(FILE *f, int nchap, int frame, TCHAR *title, INPUT_INFO *iip) {
	LONGLONG t,h,m;
	double s;

	t = (LONGLONG)frame * 10000000 * iip->scale / iip->rate;
	h = t / 36000000000;
	m = (t - h * 36000000000) / 600000000;
	s = (t - h * 36000000000 - m * 600000000) / 10000000.0;

	fprintf(f, "CHAPTER%02d=%02d:%02d:%06.3f\n", nchap, (int)h, (int)m, s);
	fprintf(f, "CHAPTER%02dNAME=%s\n", nchap, title);
	fflush(f);
}

int _tmain(int argc, _TCHAR* argv[])
{
	// メモリリークチェック
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	printf(_T("chapter.auf pre loading program.\n"));
	printf(_T("usage:\n"));
	printf(_T("\tchapter_exe.exe -v input_avs -o output_txt\n"));
	printf(_T("params:\n\t-v 入力画像ファイル\n\t-a 入力音声ファイル（省略時は動画と同じファイル）\n\t-m 無音判定閾値（1～2^15)\n\t-s 最低無音フレーム数\n"));

	TCHAR *avsv = NULL;
	TCHAR *avsa = NULL;
	TCHAR *out =  NULL;
	short setmute = 50;
	int setseri = 10;

	for(int i=1; i<argc-1; i++) {
		char *s	= argv[i];
		if (s[0] == '-') {
			switch(s[1]) {
			case 'v':
				avsv = argv[i+1];
				if (strlen(s) > 2 && s[2] == 'a') {
					avsa = argv[i+1];
				}
				i++;
				break;
			case 'a':
				avsa = argv[i+1];
				i++;
				break;
			case 'o':
				out = argv[i+1];
				i++;
				break;
			case 'm':
				setmute = atoi(argv[i+1]);
				i++;
				break;
			case 's':
				setseri = atoi(argv[i+1]);
				i++;
				break;
			default:
				printf("error: unknown param: %s\n", s);
				break;
			}
		} else {
			printf("error: unknown param: %s\n", s);
		}
	}

	// 音声入力が無い場合は動画内にあると仮定
	if (avsa == NULL) {
		avsa = avsv;
	}

	if (out == NULL) {
		printf("error: no output file path!");
		return -1;
	}

	printf(_T("Setting\n"));
	printf(_T("\tvideo: %s\n\taudio: %s\n\tout: %s\n"), avsv, (strcmp(avsv, avsa) ? avsa : "(within video source)"), out);
	printf(_T("\tmute: %d\n\tseri: %d\n"), setmute, setseri);

	printf("Loading plugins.\n");

	Source *video = NULL;
	Source *audio = NULL;
	try {
		AuiSource *srcv = new AuiSource();
		srcv->init(avsv);
		if (srcv->has_video() == false) {
			srcv->release();
			throw "Error: No Video Found!";
		}
		video = srcv;
		// 同じソースの場合は同じインスタンスで読み込む
		if (strcmp(avsv, avsa) == 0 && srcv->has_audio()) {
			audio = srcv;
			audio->add_ref();
		}

		// 音声が別ファイルの時
		if (audio == NULL) {
			if (strlen(avsa) > 4 && _stricmp(".wav", avsa + strlen(avsa) - 4) == 0) {
				// wav
				WavSource *wav = new WavSource();
				wav->init(avsa);
				if (wav->has_audio()) {
					audio = wav;
					audio->set_rate(video->get_input_info().rate, video->get_input_info().scale);
				} else {
					wav->release();
				}
			} else {
				// aui
				AuiSource *aud = new AuiSource();
				aud->init(avsa);
				if (aud->has_audio()) {
					audio = aud;
					audio->set_rate(video->get_input_info().rate, video->get_input_info().scale);
				} else {
					aud->release();
				}
			}
		}
		
		if (audio == NULL) {
			throw "Error: No Audio!";
		}
	} catch(char *s) {
		if (video) {
			video->release();
		}
		printf("%s\n", s);
		return -1;
	}

	FILE *fout;
	if (fopen_s(&fout, out, "w") != 0) {
		printf("Error: output file open failed.");
		video->release();
		audio->release();
		return -1;
	}

	INPUT_INFO &vii = video->get_input_info();
	INPUT_INFO &aii = audio->get_input_info();

	printf(_T("Movie data\n"));
	printf(_T("\tVideo Frames: %d [%.02ffps]\n"), vii.n, (double)vii.rate / vii.scale);
	DWORD fcc = vii.handler;
	printf(_T("\tVideo Format: %c%c%c%c\n"), fcc & 0xFF, fcc >> 8 & 0xFF, fcc >> 16 & 0xFF, fcc >> 24);

	printf(_T("\tAudio Samples: %d [%dHz]\n"), aii.audio_n, aii.audio_format->nSamplesPerSec);

	if (fcc == 0x32424752 || fcc == 0x38344359) {
		printf(_T("Error: Unsupported color RGB/YC48."));
	}

	if (fcc != 0x32595559) {
		printf(_T("warning: only YUY2 is supported. continues...\n"));
		//return -1;
	}

	short buf[4800*2]; // 10fps以上
	int n = vii.n;

	// FAW check
	do {
		CFAW cfaw;
		int faws = 0;

		for (int i=0; i<min(90, n); i++) {
			int naudio = audio->read_audio(i, buf);
			int j = cfaw.findFAW(buf, naudio);
			if (j != -1) {
				cfaw.decodeFAW(buf+j, naudio-j, buf); // test decode
				faws++;
			}
		}
		if (faws > 5) {
			if (cfaw.isLoadFailed()) {
				printf("  Error: FAW detected, but no FAWPreview.auf.\n");
			} else {
				printf("  FAW detected.\n");
				audio = new FAWDecoder(audio);
			}
		}
	} while(0);

	printf(_T("--------\nStart searching...\n"));

	short mute = setmute;
	int seri = 0;
	int idx = 1;
	int frames[500];

	// start searching
	for (int i=0; i<n; i++) {
		// searching foward frame
		if (seri == 0) {
			int naudio = audio->read_audio(i+setseri-1, buf);
			naudio *= aii.audio_format->nChannels;

			bool skip = false;
			for (int j=0; j<naudio; ++j) {
				if (abs(buf[j]) > mute) {
					skip = true;
					break;
				}
			}
			if (skip) {
				i += setseri;
			}
		}

		bool nomute = false;
		int naudio = audio->read_audio(i, buf);
		naudio *= aii.audio_format->nChannels;

		for (int j=0; j<naudio; ++j) {
			if (abs(buf[j]) > mute) {
				nomute = true;
				break;
			}
		}
		if (nomute) {
			// owata
			if (seri >= setseri) {
				int start_fr = i - seri;

				printf(_T("mute%2d: %d - %dフレーム\n"), idx, start_fr, seri);

				int w = vii.format->biWidth & 0xFFFFFFF0;
				int h = vii.format->biHeight & 0xFFFFFFF0;
				unsigned char *pix0 = (unsigned char*)_aligned_malloc(1920*1088, 32);
				unsigned char *pix1 = (unsigned char*)_aligned_malloc(1920*1088, 32);
				video->read_video_y8(start_fr, pix0);

				int max_mvec = 0;
				int max_pos = start_fr;
				for (int x=start_fr+1; x<min(i, start_fr+300); x++) {
					video->read_video_y8(x, pix1);
					int cmvec = mvec( pix1, pix0, w, h, (100-0)*(100/FIELD_PICTURE), FIELD_PICTURE);
					if (max_mvec < cmvec) {
						max_mvec = cmvec;
						max_pos = x;
					}
					unsigned char *tmp = pix0;
					pix0 = pix1;
					pix1 = tmp;
				}
				frames[idx] = max_pos;

				char *mark = "";
				if (idx > 1 && abs(max_pos - frames[idx-1] - 30*15) < 30) {
					mark = "★";
				} else if (idx > 1 && abs(max_pos - frames[idx-1] - 30*30) < 30) {
					mark = "★★";
				} else if (idx > 1 && abs(max_pos - frames[idx-1] - 30*45) < 30) {
					mark = "★★★";
				} else if (idx > 1 && abs(max_pos - frames[idx-1] - 30*60) < 30) {
					mark = "★★★★";
				}
				printf("\t SCPos: %d %s\n", max_pos, mark);

				TCHAR title[256];
				sprintf_s(title, _T("%dフレーム %s SCPos:%d"), seri, mark, max_pos);

				write_chapter(fout, idx, i-seri, title, &vii);
				idx++;

				_aligned_free(pix0);
				_aligned_free(pix1);
			}
			seri = 0;
		} else {
			seri++;
		}
	}

	// ソースを解放
	video->release();
	audio->release();

	return 0;
}

