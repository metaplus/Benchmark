#include "stdafx.h"

//#define __STDC_CONSTANT_MACROS

int main0(int argc, char* argv[])
{
    int	i, videoindex;
    unsigned char *out_buffer;
    AVPacket packet = *av_packet_alloc(); //av_init_packet(&packet);
    int y_size;
    int ret;
    struct SwsContext *img_convert_ctx;

    char filepath[] = "D:/Media/NewYork.mp4";

    FILE *fp_yuv = fopen("D:/Media/NewYork.yuv", "wb+");

    av_register_all();
    avformat_network_init();
    AVFormatContext	* pFormatCtx = avformat_alloc_context();

    if (avformat_open_input(&pFormatCtx, filepath, nullptr, nullptr) != 0) {
        printf("Couldn't open input stream.\n");
        return -1;
    }
    if (avformat_find_stream_info(pFormatCtx, nullptr)<0) {
        printf("Couldn't find stream information.\n");
        return -1;
    }
    videoindex = av_find_best_stream(pFormatCtx,AVMEDIA_TYPE_VIDEO,-1,-1,nullptr,0);

    if (videoindex == -1) {
        printf("Didn't find a video stream.\n");
        return -1;
    }
    AVStream *videostream = pFormatCtx->streams[videoindex];
    AVCodec *pCodec = avcodec_find_decoder(videostream->codecpar->codec_id);
    AVCodecContext *pCodecCtx = avcodec_alloc_context3(pCodec); //maybe nullptr for pCodec
    avcodec_parameters_to_context(pCodecCtx, videostream->codecpar);

    if (pCodec == nullptr) {
        printf("Codec not found.\n");
        return -1;
    }
    if (avcodec_open2(pCodecCtx, pCodec, nullptr)<0) {
        printf("Could not open codec.\n");
        return -1;
    }
    AVFrame	*pFrame = av_frame_alloc();  //  av_image_alloc()
    av_image_alloc(pFrame->data, pFrame->linesize,
        pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, 1);
//    av_frame_make_writable()
    AVFrame	*pFrameYUV = av_frame_alloc();
    std::cerr << "buffer size " << av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1) << std::endl;
    out_buffer = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1));
    av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer,
        AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);

    //Output Info-----------------------------
    printf("--------------- File Information ----------------\n");
    av_dump_format(pFormatCtx, 0, filepath, 0);
    printf("-------------------------------------------------\n");
    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
        pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

    while (av_read_frame(pFormatCtx, &packet) >= 0) {
        if (packet.stream_index == videoindex) {
            ret = avcodec_send_packet(pCodecCtx, &packet);
            if (ret < 0) {
                fprintf(stderr, "Error during decoding\n");
                return ret;
            }
            while(ret>=0)
            {
            //    av_frame_make_writable(pFrame);
                ret = avcodec_receive_frame(pCodecCtx, pFrame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
              //      av_frame_free(&pFrame);
                    break;  //?
                }
                if (ret < 0) {
                    fprintf(stderr, "Error while decoding\n");
                    return EXIT_FAILURE;
                }
            }/*
            if(ret>0){
                sws_scale(img_convert_ctx, (const unsigned char* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
                    pFrameYUV->data, pFrameYUV->linesize);

                y_size = pCodecCtx->width*pCodecCtx->height;
                fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);    //Y 
                fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);  //U
                fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);  //V
                printf("Succeed to decode 1 frame!\n");

            }*/
        }            
        av_packet_unref(&packet);
        //av_free_packet(packet);
    }
    //flush decoder
    //FIX: Flush Frames remained in Codec
    while (1) {/*
        ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
        if (ret < 0)
            break;
        if (!got_picture)
            break;*/
        ret = avcodec_receive_frame(pCodecCtx, pFrame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            av_frame_free(&pFrame);
            break;  //?
        }
        if (ret < 0) {
            fprintf(stderr, "Error while decoding\n");
            break;
        }
        sws_scale(img_convert_ctx, (const unsigned char* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
            pFrameYUV->data, pFrameYUV->linesize);

        int y_size = pCodecCtx->width*pCodecCtx->height;
        fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);    //Y 
        fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);  //U
        fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);  //V

        printf("Flush Decoder: Succeed to decode 1 frame!\n");
    }

    sws_freeContext(img_convert_ctx);

    fclose(fp_yuv);

    av_frame_free(&pFrameYUV);
    av_frame_free(&pFrame);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);

    return EXIT_SUCCESS;
}