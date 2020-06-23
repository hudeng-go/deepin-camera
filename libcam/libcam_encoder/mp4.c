#include "mp4.h"
#include <libavutil/mathematics.h>

static int64_t video_pts = 0;
static int64_t audio_pts = 0;

AVFormatContext *mp4_create_context(const char *filename)
{
    AVFormatContext *mp4_ctx;
    avformat_alloc_output_context2(&mp4_ctx, NULL, NULL, filename);
    if (!mp4_ctx) {
        printf("Could not deduce output format from file extension: using MPEG.\n");
        avformat_alloc_output_context2(&mp4_ctx, NULL, "mpeg", filename);
    }
    if (!mp4_ctx)
    {
		fprintf(stderr, "ENCODER: FATAL memory allocation failure (mp4_create_context): %s\n", strerror(errno));
		exit(-1);
	}
    return mp4_ctx;
}

void mp4_add_video_stream(
		AVFormatContext *mp4_ctx,
        encoder_codec_data_t *video_codec_data,
        OutputStream *video_stream)
{

    video_stream->st = avformat_new_stream(mp4_ctx, video_codec_data->codec);

    if(!video_stream->st)
    {
        fprintf(stderr,"Could not allocate stream\n");
        exit(1);
    }
    video_stream->st->id = mp4_ctx->nb_streams - 1;
    video_stream->enc = video_codec_data->codec_context;
    video_stream->st->time_base = video_codec_data->codec_context->time_base;

    if(mp4_ctx->oformat->flags & AVFMT_GLOBALHEADER)
    {
        video_stream->enc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
}

void mp4_add_audio_stream(
		AVFormatContext *mp4_ctx,
        encoder_codec_data_t *audio_codec_data,
        OutputStream *audio_stream)
{

    audio_stream->st = avformat_new_stream(mp4_ctx,audio_codec_data->codec);
    if(!audio_stream->st)
    {
        fprintf(stderr,"Could not allocate stream\n");
        exit(1);
    }
    audio_stream->st->id = mp4_ctx->nb_streams - 1;
    audio_stream->st->time_base = audio_codec_data->codec_context->time_base;
    audio_stream->enc = audio_codec_data->codec_context;
    if(mp4_ctx->oformat->flags & AVFMT_GLOBALHEADER)
    {
        audio_stream->enc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
}

static void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt)  //log日志相关 打印一些信息 	Definition at line 70 of file muxing.c.
{
    AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

    printf("pts:%s pts_time:%s duration:%s\n",av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),av_ts2str(pkt->duration));
    /*
    printf("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
           av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
           av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
           av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
           pkt->stream_index);  //这里pts和dts都源自编码前frame.pts 而duration没有被赋值，是初始化时的0
    */
}

int mp4_write_packet(
        AVFormatContext *mp4_ctx,
        encoder_codec_data_t *codec_data,
        int stream_index,
        uint8_t *outbuf,
        uint32_t outbuf_size,
        int flags)
{
    AVPacket *outpacket = codec_data->outpkt;
    //= codec_data->outpkt;
    outpacket->data = (uint8_t*)calloc((unsigned int)outbuf_size, sizeof(uint8_t));

    memcpy(outpacket->data, outbuf, outbuf_size);
    outpacket->size =(int)outbuf_size;

    if(codec_data->codec_context->codec_type == AVMEDIA_TYPE_VIDEO){
        outpacket->pts = video_pts;
        outpacket->dts = video_pts;

        outpacket->duration = 0;
        outpacket->flags = flags;
        outpacket->stream_index = stream_index;
        AVRational video_time = mp4_ctx->streams[stream_index]->time_base;
        /*AVRational time_base = codec_data->codec_context->time_base;*/

        av_packet_rescale_ts(outpacket, (AVRational){1, 33}, video_time);
        //log_packet(mp4_ctx, outpacket);

        av_write_frame(mp4_ctx, outpacket);
        //free(outpacket->data);
        video_pts++;
    }
    if(codec_data->codec_context->codec_type == AVMEDIA_TYPE_AUDIO) {

        outpacket->pts = audio_pts;
        outpacket->flags = flags;
        outpacket->stream_index = stream_index;

        AVRational audio_time = mp4_ctx->streams[stream_index]->time_base;
        AVRational time_base = codec_data->codec_context->time_base;
        av_packet_rescale_ts(outpacket, time_base, audio_time);
        //log_packet(mp4_ctx, outpacket);

        av_write_frame(mp4_ctx, outpacket);

        //AAC音频标准是1024
        //MP3音频标准是1152
        audio_pts+= 1024;

    }
    if(outpacket->data){
        free(outpacket->data);
        outpacket->data = NULL;
    }
}


void mp4_destroy_context(AVFormatContext *mp4_ctx)
{
    if(mp4_ctx != NULL)
    {
        avformat_free_context(mp4_ctx);
        mp4_ctx = NULL;
    }
}


