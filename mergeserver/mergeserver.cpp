#include <stdio.h>

extern "C"
{
#include <libavcodec/avcodec.h>  
#include <libavformat/avformat.h>  
#include <libswscale/swscale.h>  
#include <libavutil/imgutils.h>  
};

int main(int argc, char* argv[])
{
    AVFormatContext *pFormatCtx;
    int             i, videoindex;
    AVCodecContext  *pCodecCtx;
    AVCodec         *pCodec;
    AVFrame *pFrame, *pFrameYUV;
    unsigned char *out_buffer;
    AVPacket *packet;
    int ret, got_picture;

    struct SwsContext *img_convert_ctx;
    FILE *fp_yuv;

    char filepath[] = "laiwan.flv";
    fp_yuv = fopen("output.yuv", "wb");

    av_register_all();
    avformat_network_init();

    pFormatCtx = avformat_alloc_context();

    if (avformat_open_input(&pFormatCtx, filepath, NULL, NULL) != 0)
    {
        printf("Couldn't open input stream.\n");
        return -1;
    }

    if (avformat_find_stream_info(pFormatCtx, NULL)<0){
        printf("Couldn't find stream information.\n");
        return -1;
    }

    videoindex = -1;
    for (i = 0; i < pFormatCtx->nb_streams; i++)
    {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoindex = i;
            break;
        }
    }

    if (videoindex == -1){
        printf("Didn't find a video stream.\n");
        return -1;
    }

    pCodecCtx = pFormatCtx->streams[videoindex]->codec;
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL){
        printf("Codec not found.\n");
        return -1;
    }
    if (avcodec_open2(pCodecCtx, pCodec, NULL)<0){
        printf("Could not open codec.\n");
        return -1;
    }
    pFrame = av_frame_alloc();
    pFrameYUV = av_frame_alloc();

    out_buffer = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1));
    av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer,
        AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);

    av_dump_format(pFormatCtx, 0, filepath, 0);

    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
        pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

    packet = (AVPacket *)av_malloc(sizeof(AVPacket));

    for (int i = 0; i < 100; i++)
    {
        while (1){
            if (av_read_frame(pFormatCtx, packet)<0)
            {
                printf("av_read_frame failed.\n");
                return -1;
            }

            if (packet->stream_index == videoindex)
                break;
        }
        ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
        if (ret < 0){
            printf("Decode Error.\n");
            return -1;
        }
        if (got_picture){
            sws_scale(img_convert_ctx, (const unsigned char* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);

            int y_size = pCodecCtx->width*pCodecCtx->height;
            fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);    //Y   
            fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);  //U  
            fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);  //V  

        }
        av_free_packet(packet);
    }


    sws_freeContext(img_convert_ctx);
    av_frame_free(&pFrameYUV);
    av_frame_free(&pFrame);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);

    fclose(fp_yuv);

	return 0;
}

