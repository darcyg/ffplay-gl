#include "VideoState.h"
#include <mutex>
using std::mutex;
int VideoState::read_thread()
{
	AVFormatContext *ic = 0;
	int err, i, ret;

	/**/
	int st_index[AVMEDIA_TYPE_NB];

	AVPacket pkt1, *pkt = &pkt1;
	int64_t stream_start_time;
	int pkt_in_play_range = 0;
	AVDictionaryEntry *t;
	AVDictionary **opts;
	int orig_nb_streams;
	mutex wait_mutex;
	int scan_all_pmts_set = 0;
	int64_t pkt_ts;

	memset(st_index, -1, sizeof(st_index));
	this->last_video_stream = this->video_stream = -1;
	this->last_audio_stream = this->audio_stream = -1;
	this->last_subtitle_stream = this->subtitle_stream = -1;
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

    if (genpts) {
    	/*Generate missing pts even if it requires parsing future frames. */
    	ic->flags |= AVFMT_FLAG_GENPTS;
    }

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

    if (seek_by_bytes < 0) {
    	seek_by_bytes = !!(ic->iformat->flags & AVFMT_TS_DISCONT) && strcmp("ogg", ic->iformat->name);
    }


}
