#include "stdafx.h"

static AVFormatContext *fmt_ctx;
static AVCodecContext *dec_ctx;
static int video_stream_index = -1;
static int64_t last_pts = AV_NOPTS_VALUE;

static int open_input_file(const char *filename)
{
    int ret;
    AVCodec *dec=avcodec_find_decoder_by_name("h264_cuvid");
    
    if ((ret = avformat_open_input(&fmt_ctx, filename, NULL, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
        return ret;
    }

    if ((ret = avformat_find_stream_info(fmt_ctx, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
        return ret;
    }

    /* select the video stream */
    ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);//&dec, 0);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find a video stream in the input file\n");
        return ret;
    }
    video_stream_index = ret;
    
    
    /* create decoding context */
    dec_ctx = avcodec_alloc_context3(dec);

    if (!dec_ctx)
        return AVERROR(ENOMEM);
    avcodec_parameters_to_context(dec_ctx, fmt_ctx->streams[video_stream_index]->codecpar);
    av_opt_set_int(dec_ctx, "refcounted_frames", 1, 0);

    /* init the video decoder */
    if ((ret = avcodec_open2(dec_ctx, dec, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open video decoder\n");
        return ret;
    }

    return 0;
}



int main(int argc, char **argv)
{
    int ret;
    AVPacket packet;
    AVFrame *frame = av_frame_alloc();
    std::ofstream fout{ "D:/Media/NewYork.yuv",std::ios::binary | std::ios::trunc };
   if (!frame ) {
        perror("Could not allocate frame");
        exit(1);
    } /*
    if (argc != 2) {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        exit(1);
    }*/
    argv[1] = "D:/Media/NewYork.mp4";
    av_register_all();

    if ((ret = open_input_file(argv[1])) < 0)
        goto end;
    auto count = 0;
    /* read all packets */
    while (1) {
        if ((ret = av_read_frame(fmt_ctx, &packet)) < 0)
            break;
        std::cout << "count " << count++ << "\n";
        if (packet.stream_index == video_stream_index) {
            ret = avcodec_send_packet(dec_ctx, &packet);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Error while sending a packet to the decoder\n");
                break;
            }

            while (ret >= 0) {
                ret = avcodec_receive_frame(dec_ctx, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                }
                else if (ret < 0) {
                    av_log(NULL, AV_LOG_ERROR, "Error while receiving a frame from the decoder\n");
                    goto end;
                }

                if (ret >= 0) {
                    frame->pts = frame->best_effort_timestamp;
                    fout.write(reinterpret_cast<char*>(frame->data[0]), 3840 * 2048 );
                    fout.write(reinterpret_cast<char*>(frame->data[1]), 3840 * 2048 / 2);
                    av_frame_unref(frame);
                }
            }
        }
        av_packet_unref(&packet);
    }
end:
    avcodec_free_context(&dec_ctx);
    avformat_close_input(&fmt_ctx);
    av_frame_free(&frame);

    if (ret < 0 && ret != AVERROR_EOF) {
    //    fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
        exit(1);
    }

    exit(0);
}