// 入力クラス

#pragma once

#include "stdafx.h"

interface Source {
public:
	
	virtual void init(char *infile) = 0;

	virtual bool has_video() = 0;
	virtual bool has_audio() = 0;
	virtual INPUT_INFO &get_input_info() = 0;
	virtual void set_rate(int rate, int scale) = 0;

	virtual bool read_video_y8(int frame, unsigned char *luma) = 0;
	virtual int read_audio(int frame, short *buf) = 0;
};