#include "Frame.h"

void Frame::unref_item()
{
	av_frame_unref(frame);
	avsubtitle_free(&sub);
}
void Frame::free_picture()
{

}
