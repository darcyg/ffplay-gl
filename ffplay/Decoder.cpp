#include "Decoder.h"
#include "PacketQueue.h"
#include "FrameQueue.h"
#include <assert.h>
#include <string.h>
void Decoder::init(AVCodecContext * avctx,shared_ptr<PacketQueue> queue,shared_ptr<condition_variable> empty_queue_cond)
{
	memset(&pkt,0,sizeof(AVPacket));
	memset(&pkt_temp,0,sizeof(AVPacket));
	this->queue = queue;
	this->avctx = avctx;
	pkt_serial = 0;
	finished = 0;
	packet_pending = 0;
	this->empty_queue_cond = empty_queue_cond;
	start_pts = AV_NOPTS_VALUE;
	memset(&start_pts_tb,0,sizeof(AVRational));
	next_pts = 0;
	memset(&next_pts_tb,0,sizeof(AVRational));
}

int Decoder::decode_frame(AVFrame * frame,AVSubtitle * sub)
{
	int got_frame = 0;

	auto queue = this->queue.lock();
	assert(queue);
	auto empty_queue_cond = this->empty_queue_cond.lock();
	assert(empty_queue_cond);

	do {
		/*成功解码的packet数据字节大小*/
		int ret = -1;

		if(queue->abort_request) {
			return -1;
		}
		if(!packet_pending || queue->serial != pkt_serial) {
			/*没有有效的缓存packet或者packet_queue的serial和缓存的packet的serial不同*/

			AVPacket pkt;
			do {
				if(queue->packets.empty()) {
					empty_queue_cond->notify_all();
				}
				if(queue->get(&pkt,1,&pkt_serial) < 0) {
					return -1;
				}
				if(pkt.data == PacketQueue::flush_ptk.data) {
					/*新取到的packet为哨兵packet*/

					avcodec_flush_buffers(avctx);
					finished = 0;
					next_pts = start_pts;
					next_pts_tb = start_pts_tb;
				}

				/*新取到的packet为哨兵packet或者其serial与packet_queue
				 * 的serial不同则认定该packet无效，继续从队列中取packet*/
			} while(pkt.data == PacketQueue::flush_ptk.data || queue->serial != pkt_serial);

			/*释放对旧缓存packet的引用，并更新缓存packet*/
			av_packet_unref(&this->pkt);
			pkt_temp = this->pkt = pkt;

			/*暂且认定缓存的packet会在下一次解码工作时仍然有效*/
			packet_pending = 1;
		}

		switch(avctx->codec_type) {
			case AVMEDIA_TYPE_VIDEO:
				ret = avcodec_decode_video2(avctx,frame,&got_frame,&pkt_temp);
				if(got_frame) {
					frame->pts = av_frame_get_best_effort_timestamp(frame);
				}
				break;
			case AVMEDIA_TYPE_AUDIO:
				ret = avcodec_decode_audio4(avctx,frame,&got_frame,&pkt_temp);
				if(got_frame) {
					AVRational tb = (AVRational){1,frame->sample_rate};

					/*以音频采样间隔作为时间基*/
					if(frame->pts != AV_NOPTS_VALUE) {
						frame->pts = av_rescale_q(frame->pts,av_codec_get_pkt_timebase(avctx),tb);
					}
					else if(next_pts != AV_NOPTS_VALUE) {
						frame->pts = av_rescale_q(next_pts,next_pts_tb,tb);
					}

					if(frame->pts != AV_NOPTS_VALUE) {
						next_pts = frame->pts + frame->nb_samples;
						next_pts_tb = tb;
					}
				}
				break;
			case AVMEDIA_TYPE_SUBTITLE:
				ret = avcodec_decode_subtitle2(avctx,sub,&got_frame,&pkt_temp);
				break;
		}

		if(ret < 0) {
			/*解码过程中发生错误，缓存的packet设为无效*/
			packet_pending = 0;
		}
		else {
			pkt_temp.dts = pkt_temp.pts = AV_NOPTS_VALUE;
			if(pkt_temp.data) {
				if(avctx->codec_type != AVMEDIA_TYPE_AUDIO) {
					/*若packet的类型不是音频包，则认为一次解码操作就已经将
					 * packet中的所有数据消耗完了*/
					ret = pkt_temp.size;
				}

				pkt_temp.data += ret;	/*指向还未解码的数据的位置*/
				pkt_temp.size -= ret;	/*还未解码的数据量*/
				if(pkt_temp.size <= 0) {
					/*packet中没有还未解码的数据，因此packet设为无效*/
					packet_pending = 0;
				}
			}
			else if(!got_frame){
				/*解码操作成功但没有得到frame数据*/

				packet_pending = 0;	/*设置packet无效*/
				finished = pkt_serial;	/*???*/
			}
		}

		/*没有获取到有效的frame且???*/
	} while(!got_frame && !finished);
	return got_frame;
}
void Decoder::destroy()
{
	av_packet_unref(&pkt);
	avcodec_free_context(&avctx);
}
void Decoder::abort(shared_ptr<FrameQueue> fq)
{
	auto queue = this->queue.lock();
	assert(queue);
	queue->abort();
	fq->signal();
	decoder_tid.join();
	queue->flush();
}

int Decoder::start(function<void()> fn)
{
	auto queue = this->queue.lock();
	assert(queue);
	queue->start();
	decoder_tid = std::move(thread(fn));
	return 0;
}
