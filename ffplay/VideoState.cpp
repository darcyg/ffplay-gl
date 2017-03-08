#include "VideoState.h"
#include "Options"
#include "Utils.h"
#include <mutex>

extern "C" {
    #include "cmdutils.h"
}

using std::mutex;

int VideoState::read_thread()
{
	AVFormatContext *ic = 0;
	int err, ret;

	/*多媒体流索引*/
	int video_st_index;
    int audio_st_index;

	AVPacket pkt1, *pkt = &pkt1;
	int64_t stream_start_time;
	int pkt_in_play_range = 0;
	AVDictionaryEntry *t;
	AVDictionary **opts;
	int orig_nb_streams;
	mutex wait_mutex;
	int scan_all_pmts_set = 0;
	int64_t pkt_ts;

	
	this->eof = 0;

    
	ic = avformat_alloc_context();
	if (!ic) {
		av_log(NULL, AV_LOG_FATAL, "Could not allocate context.\n");
        ret = AVERROR(ENOMEM);
		goto fail;
	}
    
	/*设置avformat_open_input中断的处理回调及回调参数*/
	ic->interrupt_callback.callback = decode_interrupt_cb;
	ic->interrupt_callback.opaque = is;

	if (!av_dict_get(format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE)) {
		/*如果format_opts中找不到key为scan_all_pmts的项，则新增该项，AV_DICT_MATCH_CASE表明需要区分大小写*/
		av_dict_set(&format_opts, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE);

		scan_all_pmts_set = 1;
	}
	/*以filename为输入源，iformat为指定格式，format_opts为选项打开文件*/
	err = avformat_open_input(&ic, this->filename, this->iformat, &format_opts);
	if (err < 0) {
		print_error(filename, err);
		ret = -1;
		goto fail;
	}
    if (scan_all_pmts_set) {
    	/*如果刚才新增了key为scan_all_pmts的项，则回删该项*/
    	av_dict_set(&format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE);
    }
    if ((t = av_dict_get(format_opts, "", NULL, AV_DICT_IGNORE_SUFFIX))) {
    	/*format_opts中包含了一些avformat_open_input函数不支持的选项*/
		av_log(NULL, AV_LOG_ERROR, "Option %s not found.\n", t->key);
		ret = AVERROR_OPTION_NOT_FOUND;
		goto fail;
	}
    this->ic = ic;
    av_format_inject_global_side_data(ic);
    
    
    
    
    
    
    
    
    
    /*从全局字典codec_opts找到每一个流的对应的编解码选项*/
    opts = setup_find_stream_info_opts(ic, codec_opts);

    orig_nb_streams = ic->nb_streams;

    /*读取流信息，赋值到AVStream*/
    err = avformat_find_stream_info(ic, opts);

    for (i = 0; i < orig_nb_streams; i++) {
    	av_dict_free(&opts[i]);
    }
    av_freep(&opts);

    if (err < 0) {
        av_log(NULL, AV_LOG_WARNING,
               "%s: could not find codec parameters\n", is->filename);
        ret = -1;
        goto fail;
    }

    
    
    
    
    
    if (ic->pb) {
    	ic->pb->eof_reached = 0;
    }
    this->realtime = is_realtime(ic);
    
    

    video_st_index = av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    audio_st_index = av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if(video_st_index >= 0) {
        //todo : query picture size
        
    }
    if(audio_st_index >= 0) {
        
    }
    if(video_st_index < 0 && audio_st_index < 0) {
        ret = -1;
        goto fail;
    }
    
    while (true) {
        if(abort_request) {
            break;
        }
        if(paused != last_paused) {
            last_paused = paused;
            if(paused) {
                /*暂停*/
                read_pause_return = av_read_pause(ic);
            }
            else {
                av_read_play(ic);
                /*继续*/
            }
        }
        if(seek_req) {
            
        }
    }

}

int VideoState::stream_component_open(int stream_index)
{
    AVCodecContext * avctx;
    AVCodec * codec;
    AVDictionary *opts = NULL;
    AVDictionaryEntry *t = NULL;
    int ret = 0;
    
    avctx = avcodec_alloc_context3(NULL);
    if(!avctx) {
        return AVERROR(ENOMEM);
    }
    
    /*复制AVStream信息到AVCodecContext*/
    ret = avcodec_parameters_to_context(avctx, ic->streams[stream_index]->codecpar);
    if(ret < 0) {
        goto fail;
    }
    av_codec_set_pkt_timebase(avctx, ic->streams[stream_index]->time_base);
    
    /*获取对应的解码器*/
    codec = avcodec_find_decoder(avctx->codec_id);
    
    avctx->codec_id = codec->id;
    /*从codec_opts中过滤出对指定codec有效的选项并返回*/
    opts = filter_codec_opts(codec_opts, avctx->codec_id, ic, ic->streams[stream_index], codec);
    
    if (!av_dict_get(opts, "threads", NULL, 0)) {
        av_dict_set(&opts, "threads", "auto", 0);
    }
    if (avctx->codec_type == AVMEDIA_TYPE_VIDEO || avctx->codec_type == AVMEDIA_TYPE_AUDIO) {
        av_dict_set(&opts, "refcounted_frames", "1", 0);
    }
    
    /*AVCodctContext关联到AVCodec*/
    ret = avcodec_open2(avctx, codec, &opts);
    if (ret < 0) {
        goto fail;
    }
    
    if ((t = av_dict_get(opts, "", NULL, AV_DICT_IGNORE_SUFFIX))) {
        av_log(NULL, AV_LOG_ERROR, "Option %s not found.\n", t->key);
        ret =  AVERROR_OPTION_NOT_FOUND;
        goto fail;
    }
    
    eof = 0;
    ic->streams[stream_index]->discard = AVDISCARD_DEFAULT;
    
    switch(avctx->codec_type) {
        case AVMEDIA_TYPE_VIDEO:
            break;
        case AVMEDIA_TYPE_AUDIO:
            
            break;
    }
}
