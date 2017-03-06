#include "PacketQueue.h"

AVPacket PacketQueue::flush_ptk;
void PacketQueue::init_flush_packet()
{
	av_init_packet(&(PacketQueue::flush_ptk));
	PacketQueue::flush_ptk.data = (uint8_t *)(&(PacketQueue::flush_ptk));
}

int PacketQueue::put_private(AVPacket * pkt)
{
	if(abort_request) {
		return -1;
	}
	if(pkt == &(PacketQueue::flush_ptk)) {
		++ serial;
	}
	auto pair = std::make_pair(*pkt,serial);
	packets.push(pair);
	duration += pkt->duration;
	cond.notify_all();
	return 0;
}

int PacketQueue::put(AVPacket * pkt)
{
	int ret;
	{
		std::lock_guard<mutex> lg(mtx);
		ret = put_private(pkt);
	}
	if(pkt != &(PacketQueue::flush_ptk) && ret < 0) {
		av_packet_unref(pkt);
	}
	return ret;
}

int PacketQueue::put_nullpacket(int stream_index)
{
	AVPacket pkt;
	AVPacket * pkt_ptr = &pkt;
	av_init_packet(pkt_ptr);
	pkt_ptr->data = NULL;
	pkt_ptr->size = 0;
	pkt_ptr->stream_index = stream_index;
	return put(pkt_ptr);
}

void PacketQueue::init()
{
	abort_request = 1;
}

void PacketQueue::flush()
{
	{
		std::lock_guard<mutex> lg(mtx);
		while(!packets.empty()) {
			AVPacket pkt = packets.front().first;
			av_packet_unref(&pkt);
			av_freep(&pkt);
			packets.pop();
		}
		duration = 0;
	}
}

void PacketQueue::destroy()
{
	flush();
}

void PacketQueue::abort()
{
	{
		std::lock_guard<mutex> lg(mtx);
		abort_request = 1;
		cond.notify_all();
	}
}

void PacketQueue::start()
{
	{
		std::lock_guard<mutex> lg(mtx);
		abort_request = 0;
		put_private(&(PacketQueue::flush_ptk));
	}
}

int PacketQueue::get(AVPacket * pkt,int block,int * serial)
{
	int ret;
	{
		std::unique_lock<mutex> ul(mtx);
		while(true) {
			if(abort_request) {
				ret = -1;
				break;
			}
			if(!packets.empty()) {
				auto pair = packets.front();
				duration -= pair.first.duration;
				*pkt = pair.first;
				if(serial) {
					*serial = pair.second;
				}
				ret = 1;
				break;
			}
			else if(!block) {
				ret = 0;
				break;
			}
			else {
				cond.wait(ul);
			}
		}
	}
	return ret;
}

