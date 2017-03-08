#include "Utils.h"
extern "C" {
    #include "libavformat/avformat.h"
}
static int is_realtime(AVFormatContext *s)
{
    if(!strcmp(s->iformat->name, "rtp") ||
       !strcmp(s->iformat->name, "rtsp") ||
       !strcmp(s->iformat->name, "sdp"))
    {
        return 1;
    }
    
    
    if(s->pb &&
       (!strncmp(s->filename, "rtp:", 4) ||
        !strncmp(s->filename, "udp:", 4)))
    {
        return 1;   
    }
    
    return 0;
}

