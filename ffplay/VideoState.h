#ifndef VIDEO_STATE_H
#define VIDEO_STATE_H

#include <thread>
#include <condition_variable>


extern "C" {
    #include "libavformat/avformat.h"
}


using std::thread;
using std::condition_variable;

class VideoState
{
public:
	int read_thread();
    int stream_component_open(int stream_index);
	thread read_thread;
    
    AVInputFormat *iformat;
    int abort_request;
    int force_refresh;
    int paused;
    int last_paused;
    int queue_attachments_req;
    int seek_req;
    int seek_flags;
    int64_t seek_pos;
    int64_t seek_rel;
    int read_pause_return;
    AVFormatContext *ic;
    
    /*是否流媒体流*/
    int realtime;

    Clock audclk;
    Clock vidclk;

    FrameQueue pictq;
    FrameQueue sampq;

    Decoder auddec;
    Decoder viddec;


    double audio_clock;
    int audio_clock_serial;
    double audio_diff_cum; /* used for AV difference average computation */
    double audio_diff_avg_coef;
    double audio_diff_threshold;
    int audio_diff_avg_count;
    AVStream *audio_st;
    PacketQueue audioq;
    int audio_hw_buf_size;
    uint8_t *audio_buf;
    uint8_t *audio_buf1;
    unsigned int audio_buf_size; /* in bytes */
    unsigned int audio_buf1_size;
    int audio_buf_index; /* in bytes */
    int audio_write_buf_size;
    int audio_volume;
    int muted;
    struct AudioParams audio_src;
    struct AudioParams audio_tgt;
    struct SwrContext *swr_ctx;
    int frame_drops_early;
    int frame_drops_late;

    enum ShowMode {
        SHOW_MODE_NONE = -1, SHOW_MODE_VIDEO = 0, SHOW_MODE_WAVES, SHOW_MODE_RDFT, SHOW_MODE_NB
    } show_mode;
    int16_t sample_array[SAMPLE_ARRAY_SIZE];
    int sample_array_index;
    int last_i_start;

    int rdft_bits;
    FFTSample *rdft_data;
    int xpos;
    double last_vis_time;
    SDL_Texture *vis_texture;
    SDL_Texture *sub_texture;


    double frame_timer;
    double frame_last_returned_time;
    double frame_last_filter_delay;
    AVStream *video_st;
    PacketQueue videoq;
    
    int eof;

    char *filename;
    int width, height, xleft, ytop;
    int step;
    condition_variable continue_read_thread;
};

#endif
