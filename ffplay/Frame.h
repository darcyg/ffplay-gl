#ifndef FRAME_H
#define FRAME_H

extern "C" {
#include "libavformat/avformat.h"
}

class Frame
{
public:
	void unref_item();
	void free_picture();

	int serial;
	AVFrame * frame;
	AVSubtitle sub;
	double pts;
	double duration;
	int64_t pos;
	//GLuint bmp;
	int allocated;
	int width;
	int height;
	int format;
	AVRational sar;
	int uploaded;
	int flip_v;
};

#endif
