#include "FrameQueue.h"
#include "PacketQueue.h"
#include <assert.h>

int FrameQueue::init(shared_ptr<PacketQueue> pktq,int max_size,int keep_last)
{
	this->pktq = pktq;
	this->max_size = FFMIN(max_size,FRAME_QUEUE_SIZE);
	this->keep_last = keep_last;
	for(int i=0;i<this->max_size;++i) {
		if(!(queue[i].frame = av_frame_alloc())) {
			return AVERROR(ENOMEM);
		}
	}
	return 0;
}
void FrameQueue::destroy()
{
	for(int i=0;i<max_size;++i) {
		Frame * vp = &(queue[i]);
		vp->unref_item();
		av_frame_free(&vp->frame);
		vp->free_picture();
	}
}
void FrameQueue::signal()
{
	{
		std::lock_guard<mutex> lg(mtx);
		cond.notify_all();
	}
}

Frame * FrameQueue::peek()
{
	return &(queue[(rindex + rindex_shown) % max_size]);
}
Frame * FrameQueue::peek_next()
{
	return &(queue[(rindex + rindex_shown + 1) % max_size]);
}
Frame * FrameQueue::peek_last()
{
	return &(queue[rindex]);
}
Frame * FrameQueue::peek_writable()
{
	auto pktq = this->pktq.lock();
	assert(pktq);
	{
		std::unique_lock<mutex> ul(mtx);
		while(size >= max_size && !pktq->abort_request) {
			cond.wait(ul);
		}
	}
	if(pktq->abort_request) {
		return NULL;
	}
	return &(queue[windex]);
}
Frame * FrameQueue::peek_readable()
{
	auto pktq = this->pktq.lock();
	assert(pktq);
	{
		std::unique_lock<mutex> ul(mtx);
		while(size - rindex_shown <= 0 && !pktq->abort_request) {
			cond.wait(ul);
		}
	}
	if(pktq->abort_request) {
		return NULL;
	}
	return &(queue[(rindex + rindex_shown) % max_size]);
}
void FrameQueue::push()
{
	if(++windex == max_size) {
		windex = 0;
	}
	{
		std::lock_guard<mutex> lg(mtx);
		++ size;
		cond.notify_all();
	}
}
void FrameQueue::next()
{
	if(keep_last && !rindex_shown) {
		rindex_shown = 1;
		return;
	}
	queue[rindex].unref_item();
	if(++rindex == max_size) {
		rindex = 0;
	}
	{
		std::lock_guard<mutex> lg(mtx);
		-- size;
		cond.notify_all();
	}
}
int FrameQueue::nb_remaining()
{
	return size - rindex_shown;
}
int64_t FrameQueue::last_pos()
{
	auto pktq = this->pktq.lock();
	if(!pktq) {
		exit(-1);
	}
	if(rindex_shown && queue[rindex].serial == pktq->serial) {
		return queue[rindex].pos;
	}
	return -1;
}
