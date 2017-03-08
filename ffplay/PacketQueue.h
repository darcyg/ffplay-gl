#ifndef PACKET_QUEUE_H
#define PACKET_QUEUE_H

#include <queue>
#include <utility>
#include <condition_variable>
#include <mutex>

extern "C" {
    #include "libavformat/avformat.h"
}


using std::pair;
using std::queue;
using std::mutex;
using std::condition_variable;

class PacketQueue
{
public:
	static AVPacket flush_ptk;
	static void init_flush_packet();

	int put(AVPacket * pkt);
	int put_nullpacket(int stream_index);
	void init();
	void flush();
	void destroy();
	void abort();
	void start();
	int get(AVPacket *pkt,int block,int * serial);
	int put_private(AVPacket * pkt);

	bool abort_request;
	int serial;
	queue<pair<AVPacket,int>> packets;
	int64_t duration;
	mutex mtx;
	condition_variable cond;
};

#endif
