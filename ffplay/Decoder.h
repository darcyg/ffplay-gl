#ifndef DECODER_H
#define DECODER_H

#include <memory>
#include <thread>
#include <condition_variable>
#include <functional>

extern "C" {
#include "libavformat/avformat.h"
}

using std::shared_ptr;
using std::weak_ptr;
using std::thread;
using std::condition_variable;
using std::function;

class PacketQueue;
class FrameQueue;

class Decoder
{
public:
	void init(AVCodecContext * avctx,shared_ptr<PacketQueue> queue,shared_ptr<condition_variable> empty_queue_cond);
	int decode_frame(AVFrame * frame,AVSubtitle * sub);
	void destroy();
	void abort(shared_ptr<FrameQueue> fq);
	int start(function<void()> fn);


	AVPacket pkt;
	AVPacket pkt_temp;
	weak_ptr<PacketQueue> queue;
	AVCodecContext *avctx;
	int pkt_serial;
	int finished;
	int packet_pending;
	weak_ptr<condition_variable> empty_queue_cond;
	int64_t start_pts;
	AVRational start_pts_tb;
	int64_t next_pts;
	AVRational next_pts_tb;
	thread decoder_tid;
};

#endif
