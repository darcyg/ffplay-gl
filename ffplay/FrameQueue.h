#ifndef FRAME_QUEUE_H
#define FRAME_QUEUE_H

#include <mutex>
#include <condition_variable>
#include <memory>
#include "Frame.h"


using std::mutex;
using std::condition_variable;
using std::weak_ptr;
using std::shared_ptr;

#define VIDEO_PICTURE_QUEUE_SIZE 3
#define SUBPICTURE_QUEUE_SIZE 16
#define SAMPLE_QUEUE_SIZE 9
#define FRAME_QUEUE_SIZE FFMAX(SAMPLE_QUEUE_SIZE, FFMAX(VIDEO_PICTURE_QUEUE_SIZE, SUBPICTURE_QUEUE_SIZE))

class PacketQueue;

class FrameQueue
{
public:
	int init(shared_ptr<PacketQueue> pktq,int max_size,int keep_last);
	void destroy();
	void signal();
	
	/*返回可读帧*/
	Frame * peek();
	
	/*返回可读帧的后一帧*/
	Frame * peek_next();
	
	/*返回保留的旧帧*/
	Frame * peek_last();
	
	/*返回可写帧，队列满时等待*/
	Frame * peek_writable();
	
	/*返回可读帧，队列空时等待*/
	Frame * peek_readable();
	
	/*write index + 1*/
	void push();
	
	/*read index + 1，若keep_last为true，则第一次调用next时read index不变，且rindex_shown置为true*/
	void next();
	
	int nb_remaining();
	int64_t last_pos();

    Frame queue[FRAME_QUEUE_SIZE];
    int rindex;
    int windex;
    int size;
    int max_size;
    
    /*是否将一帧旧帧保留在队列中*/
    int keep_last;
    
    /*是否存在一帧旧帧*/
    int rindex_shown;
    
    mutex mtx;
    condition_variable cond;
    weak_ptr<PacketQueue> pktq;
};

#endif
