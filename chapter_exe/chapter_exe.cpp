// chapter_exe.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include "source.h"

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

class AuiSource : public Source {
protected:
	string _in, _plugin;

	HMODULE _dll;

	INPUT_PLUGIN_TABLE *_ipt;
	INPUT_HANDLE _ih;
	INPUT_INFO _ip;

public:
	AuiSource(void) : _dll(NULL) { }
	virtual ~AuiSource() {
		if (_dll) {
			FreeLibrary(_dll);
		}
	}
	
	virtual void init(char *infile) {
		_in = infile;
		_plugin = "avsinp.aui";

		int p = _in.find("://");
		if (p != _in.npos) {
			_plugin = _in.substr(0, p);
			_in = _in.substr(p+3);
		}

		printf(" -%s\n", _plugin.c_str());

		_dll = LoadLibrary(_plugin.c_str());
		if (_dll == NULL) {
			throw "   plugin loading failed.";
		}

		FARPROC f = GetProcAddress(_dll, _T("GetInputPluginTable"));
		if (f == NULL) {
			throw "   not Aviutl input plugin error.";
		}
		_ipt = (INPUT_PLUGIN_TABLE*)f();
		if (_ipt == NULL) {
			throw "   not Aviutl input plugin error.";
		}
		if (_ipt->func_init) {
			if (_ipt->func_init() == FALSE) {
				throw "   func_init() failed.";
			}
		}

		_ih = _ipt->func_open((LPSTR)_in.c_str());
		if (_ih == NULL) {
			throw "   func_open() failed.";
		}

		if (_ipt->func_info_get(_ih, &_ip) == FALSE) {
			throw "   func_info_get() failed...";
		}
	}

	bool has_video() {
		return (_ip.flag & INPUT_INFO_FLAG_VIDEO) != 0;
	}
	bool has_audio() {
		return (_ip.flag & INPUT_INFO_FLAG_AUDIO) != 0;
	}

	INPUT_INFO &get_input_info() {
		return _ip;
	}

	void set_rate(int rate, int scale) {
		_ip.rate = rate;
		_ip.scale = scale;
	}

	bool read_video_y8(int frame, unsigned char *luma) {
		int h = _ip.format->biHeight;
		int w = _ip.format->biWidth;
		unsigned char *buf = (unsigned char *)malloc(2 * h * w);

		int ret = _ipt->func_read_video(_ih, frame, buf);
		if (ret == 0) {
			return false;
		}

		int skip_w = w & 0x0F;
		w = w - skip_w;

		unsigned char *p = buf;
		for (int i=0; i<w; i++) {
			for (int j=0; j<h; j++) {
				*luma = *p;

				luma++;
				p += 2;
			}
			p += skip_w * 2;
		}
		free(buf);
		return true;
	}

	int read_audio(int frame, short *buf) {
		int start = (int)((double)frame * _ip.audio_format->nSamplesPerSec / _ip.rate * _ip.scale);
		int end = (int)((double)(frame + 1) * _ip.audio_format->nSamplesPerSec / _ip.rate * _ip.scale);
		return _ipt->func_read_audio(_ih, start, end - start, buf);
	}
};

class WavSource : public AuiSource {
	string _in;

	FILE *_f;
	__int64 _start;
	__int64 _end;
	WAVEFORMATEX _fmt;

public:
	WavSource() : _f(NULL), _start(0) {
	}
	~WavSource() {
		if (_f) {
			fclose(_f);
		}
	}

	void init(char *infile) {
		printf(" -WavSource\n");
		_f = fopen(infile, "rb");
		if (_f == NULL) {
			throw "   wav open failed.";
		}

		char buf[1000];
		if (fread(buf, 1, 4, _f) != 4 || strncmp(buf, "RIFF", 4) != 0) {
			throw "   no RIFF header.";
		}
		fseek(_f, 4, SEEK_CUR);
		if (fread(buf, 1, 4, _f) != 4 || strncmp(buf, "WAVE", 4) != 0) {
			throw "   no WAVE header.";
		}

		// chunk
		while(fread(buf, 1, 4, _f) == 4) {
			if (ftell(_f) > 1000000) {
				break;				
			}

			int size = 0;
			fread(&size, 4, 1, _f);
			if (strncmp(buf, "fmt ", 4) == 0) {
				if (fread(&_fmt, min(size, sizeof(_fmt)), 1, _f) != 1) {
					throw "   illegal WAVE file.";
				}
				if (_fmt.wFormatTag != WAVE_FORMAT_PCM) {
					throw "   only PCM supported.";
				}
			} else if (strncmp(buf, "data", 4) == 0){
				fseek(_f, 4, SEEK_CUR);
				_start = _ftelli64(_f);
				break;
			} else {
				fseek(_f, size, SEEK_CUR);
			}
		}
		if (_start == 0) {
			fclose(_f);
			throw "   maybe not wav file.";
		}

		ZeroMemory(&_ip, sizeof(_ip));
		_ip.flag |= INPUT_INFO_FLAG_AUDIO;
		_ip.audio_format = &_fmt;
		_ip.audio_format_size = sizeof(_fmt);
		_ip.audio_n = -1;
	}

	int read_audio(int frame, short *buf) {
		int start = (int)((double)frame * _ip.audio_format->nSamplesPerSec / _ip.rate * _ip.scale);
		int end = (int)((double)(frame + 1) * _ip.audio_format->nSamplesPerSec / _ip.rate * _ip.scale);

		_fseeki64(_f, _start + start * _fmt.nBlockAlign, SEEK_SET);

		return fread(buf, _fmt.nBlockAlign, end - start, _f);
	}
};

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

int _tmain(int argc, _TCHAR* argv[])
{
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

	if (avsa == NULL) {
		avsa = avsv;
	}

	if (out == NULL) {
		printf("error: no output file path!");
		return -1;
	}

	printf(_T("Setting\n"));
	printf(_T("\tvideo: %s\n\taudio: %s\n\tout:%s\n"), avsv, (strcmp(avsv, avsa) ? avsa : "(within video source)"), out);
	printf(_T("\tmute: %d\n\tseri: %d\n"), setmute, setseri);

	printf("Loading plugins.\n");
	
	AuiSource srcv, srca;
	WavSource srcw;
	Source *video = NULL;
	Source *audio = NULL;
	try {
		srcv.init(avsv);
		if (srcv.has_video() == false) {
			throw "Error: No Video Found!";
		}
		video = &srcv;
		// 同じソースの場合は同じインスタンスで読み込む
		if (strcmp(avsv, avsa) == 0 && srcv.has_audio()) {
			audio = &srcv;
		}

		// 音声が別ファイルの時
		if (audio == NULL) {
			if (strlen(avsa) > 4 && _stricmp(".wav", avsa + strlen(avsa) - 4) == 0) {
				// wav
				srcw.init(avsa);
				if (srcw.has_audio()) {
					audio = &srcw;
					audio->set_rate(video->get_input_info().rate, video->get_input_info().scale);
				}
			} else {
				// aui
				srca.init(avsa);
				if (srca.has_audio()) {
					audio = &srca;
					audio->set_rate(video->get_input_info().rate, video->get_input_info().scale);
				}
			}
		}
		
		if (audio == NULL) {
			throw "Error: No Audio!";
		}
	} catch(char *s) {
		printf("%s\n", s);
		return -1;
	}

	FILE *fout = fopen(out, "w");

	INPUT_INFO &vii = video->get_input_info();
	INPUT_INFO &aii = audio->get_input_info();

	printf(_T("Movie data\n"));
	printf(_T("\tVideo Frames: %d [%.02ffps]\n"), vii.n, (double)vii.rate / vii.scale);
	DWORD fcc = vii.handler;
	printf(_T("\tVideo Format: %c%c%c%c\n"), fcc & 0xFF, fcc >> 8 & 0xFF, fcc >> 16 & 0xFF, fcc >> 24);

	printf(_T("\tAudio Samples: %d [%dHz]\n"), aii.audio_n, aii.audio_format->nSamplesPerSec);

	if (fcc == 0x32424752 || fcc == 0x38344359) {
		printf(_T("Error: RGB/YC48 source not supported."));
	}

	if (fcc != 0x32595559) {
		printf(_T("warning: only YUY2 is supported. continues...\n"));
		//return -1;
	}

	printf(_T("--------\nStart searching...\n"));

	short buf[10000];

	int n = vii.n;
	short mute = setmute;
	int seri = 0;
	int idx = 1;
	int frames[500];

	// FAW check
	bool isFAW = false;
	CFAW cfaw;
	int faws = 0;

	for (int i=0; i<min(10, n); i++) {
		int naudio = audio->read_audio(i, buf);
		int j = cfaw.findFAW(buf, naudio);
		if (j != -1) {
			cfaw.decodeFAW(buf+j, naudio-j, buf); // test decode
			faws++;
		}
	}
	if (faws > 3) {
		if (cfaw.isLoadFailed()) {
			printf("  Error: FAW detected, but no FAWPreview.auf.\n");
		} else {
			printf("  FAW detected.\n");
			isFAW = true;
		}
	}

	// start searching
	for (int i=0; i<n; i++) {
		// searching foward frame
		if (seri == 0) {
			int naudio = audio->read_audio(i+setseri-1, buf);
			naudio *= aii.audio_format->nChannels;

			// decode FAW
			if (isFAW) {
				int j = cfaw.findFAW(buf, naudio);
				if (j != -1) {
					naudio = cfaw.decodeFAW(buf+j, naudio-j, buf);
				}
			}

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
		
		// decode FAW
		if (isFAW) {
			int j = cfaw.findFAW(buf, naudio);
			if (j != -1) {
				naudio = cfaw.decodeFAW(buf+j, naudio-j, buf);
			}
		}

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

				printf(_T("mute%2d: %d - %dフレーム\n"), idx, i-seri, seri);

				int w = vii.format->biWidth & 0xFFFFFFF0;
				int h = vii.format->biHeight & 0xFFFFFFF0;
				unsigned char *pix0 = (unsigned char*)_aligned_malloc(1920*1088, 32);
				unsigned char *pix1 = (unsigned char*)_aligned_malloc(1920*1088, 32);
				video->read_video_y8(i-seri, pix0);

				int max_mvec = 0;
				int max_pos = i-seri;
				for (int x=i-seri+1; x<i; x++) {
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

	//getc(stdin);
	return 0;
}

