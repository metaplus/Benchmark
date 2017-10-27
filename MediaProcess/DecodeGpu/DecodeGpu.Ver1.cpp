#include "stdafx.h"

//#define __STDC_CONSTANT_MACROS

static AVBufferRef *hw_device_ctx = NULL;
static enum AVPixelFormat hw_pix_fmt;
static FILE *output_file = NULL;
static SwsContext* sws_ctx;
static enum AVPixelFormat find_fmt_by_hw_type(const enum AVHWDeviceType type)
{
    enum AVPixelFormat fmt;

    switch (type) {
    case AV_HWDEVICE_TYPE_VAAPI:
        fmt = AV_PIX_FMT_VAAPI;
        break;
    case AV_HWDEVICE_TYPE_DXVA2:
        fmt = AV_PIX_FMT_DXVA2_VLD;
        break;
    case AV_HWDEVICE_TYPE_D3D11VA:
        fmt = AV_PIX_FMT_D3D11;
        break;
    case AV_HWDEVICE_TYPE_VDPAU:
        fmt = AV_PIX_FMT_VDPAU;
        break;
    case AV_HWDEVICE_TYPE_VIDEOTOOLBOX:
        fmt = AV_PIX_FMT_VIDEOTOOLBOX;
        break;
    case AV_HWDEVICE_TYPE_CUDA:
        fmt = AV_PIX_FMT_CUDA;
        break;
    default:
        fmt = AV_PIX_FMT_NONE;
        break;
    }

    return fmt;
}

static int hw_decoder_init(AVCodecContext *ctx, const enum AVHWDeviceType type)
{
    int err = 0;

    if ((err = av_hwdevice_ctx_create(&hw_device_ctx, type,
        nullptr, nullptr, 0)) < 0) {
        fprintf(stderr, "Failed to create specified HW device.\n");
        return err;
    }
    ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);

    return err;
}

static enum AVPixelFormat get_hw_format(AVCodecContext *ctx,
    const enum AVPixelFormat *pix_fmts)
{
    const enum AVPixelFormat *p;

    for (p = pix_fmts; *p != -1; p++) {
        if (*p == hw_pix_fmt)
            return *p;
    }

    fprintf(stderr, "Failed to get HW surface format.\n");
    return AV_PIX_FMT_NONE;
}

static int decode_write(AVCodecContext *avctx, AVPacket *packet)
{
    AVFrame *frame = NULL, *sw_frame = NULL;
    AVFrame *yuv_frame = av_frame_alloc();
    av_image_alloc(yuv_frame->data, yuv_frame->linesize,
        avctx->width, avctx->height, AV_PIX_FMT_YUV420P, 1);
    AVFrame *tmp_frame = NULL;
    uint8_t *buffer = NULL;
    int size;
    int ret = 0;

    ret = avcodec_send_packet(avctx, packet);
    if (ret < 0) {
        fprintf(stderr, "Error during decoding\n");
        return ret;
    }

    while (ret >= 0) {
        if (!(frame = av_frame_alloc()) || !(sw_frame = av_frame_alloc())) {
            fprintf(stderr, "Can not alloc frame\n");
            ret = AVERROR(ENOMEM);
            goto fail;
        }

        ret = avcodec_receive_frame(avctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            av_frame_free(&frame);
            av_frame_free(&sw_frame);
            return 0;
        }
        else if (ret < 0) {
            fprintf(stderr, "Error while decoding\n");
            goto fail;
        }
///*
        if (frame->format == hw_pix_fmt) {
            // retrieve data from GPU to CPU 
            if ((ret = av_hwframe_transfer_data(sw_frame, frame, 0)) < 0) {
                fprintf(stderr, "Error transferring the data to system memory\n");
                goto fail;
            }
            tmp_frame = sw_frame;
        }
        else
        {
            std::cerr << "not hw_pix_fmt \n";
            tmp_frame = frame;
        }
            
        
        size = av_image_get_buffer_size(
            static_cast<AVPixelFormat>(tmp_frame->format),
            tmp_frame->width,
            tmp_frame->height,
            1);
        buffer = static_cast<uint8_t*>(av_malloc(size));
        if (!buffer) {
            fprintf(stderr, "Can not alloc buffer\n");
            ret = AVERROR(ENOMEM);
            goto fail;
        }
        fwrite(tmp_frame->data[0], 1, 3840 * 2048, output_file);
        fwrite(tmp_frame->data[1], 1, 3840 * 2048 / 2, output_file);
        /*
        ret = av_image_copy_to_buffer(buffer, size,
            (const uint8_t * const *)tmp_frame->data,
            (const int *)tmp_frame->linesize,
            static_cast<AVPixelFormat>(tmp_frame->format),
            tmp_frame->width, tmp_frame->height, 1);
        if (ret < 0) {
            fprintf(stderr, "Can not copy image to buffer\n");
            goto fail;
        }
        sws_scale(sws_ctx, tmp_frame->data, tmp_frame->linesize, 0, tmp_frame->height,
            yuv_frame->data, yuv_frame->linesize);
        if ((ret = fwrite(yuv_frame->data[0], 1, size, output_file)) < 0) {
            fprintf(stderr, "Failed to dump raw data.\n");
            goto fail;
        }*/

    fail:
        av_frame_free(&frame);
        av_frame_free(&sw_frame);
        av_frame_free(&yuv_frame);
        if (buffer)
            av_freep(&buffer);
        if (ret < 0)
            return ret;
    }

    return 0;
}

int main1(int argc, char *argv[])
{
    auto time_base = std::chrono::steady_clock::now();
    AVFormatContext *input_ctx = nullptr;
    int video_stream, ret;
    AVStream *video = nullptr;
    AVCodecContext *decoder_ctx = nullptr;
    AVCodec *decoder = nullptr;
    AVPacket packet;
    enum AVHWDeviceType type;

  //  if (argc < 4) {
  //      fprintf(stderr, "Usage: %s <vaapi|vdpau|dxva2|d3d11va> <input file> <output file>\n", argv[0]);
  //      return -1;
  //  }

    av_register_all();
    argv[1] = "dxva2";
    argv[2] = "D:/Media/NewYork.mp4";
    argv[3] = "D:/Media/NewYork.yuv";

    type = av_hwdevice_find_type_by_name(argv[1]);
    hw_pix_fmt = find_fmt_by_hw_type(type);
    
    if (hw_pix_fmt == -1) {
        fprintf(stderr, "Cannot support '%s' in this example.\n", argv[1]);
        return -1;
    }

    /* open the input file */
    if (avformat_open_input(&input_ctx, argv[2], nullptr, nullptr) != 0) {
        fprintf(stderr, "Cannot open input file '%s'\n", argv[2]);
        return -1;
    }

    if (avformat_find_stream_info(input_ctx, nullptr) < 0) {
        fprintf(stderr, "Cannot find input stream information.\n");
        return -1;
    }

    /* find the video stream information */
    ret = av_find_best_stream(input_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
    if (ret < 0) {
        fprintf(stderr, "Cannot find a video stream in the input file\n");
        return -1;
    }
    video_stream = ret;
   
    if (!(decoder_ctx = avcodec_alloc_context3(decoder)))
        return AVERROR(ENOMEM);

    video = input_ctx->streams[video_stream];
    if (avcodec_parameters_to_context(decoder_ctx, video->codecpar) < 0)
        return -1;

    decoder_ctx->get_format = get_hw_format;
    
    av_opt_set_int(decoder_ctx, "refcounted_frames", 1, 0);

    if (hw_decoder_init(decoder_ctx, type) < 0)
        return -1;

    if ((ret = avcodec_open2(decoder_ctx, decoder, nullptr)) < 0) {
        fprintf(stderr, "Failed to open codec for stream #%u\n", video_stream);
        return -1;
    }
    sws_ctx = sws_getContext(
        decoder_ctx->width, decoder_ctx->height, AV_PIX_FMT_NV12,
        decoder_ctx->width, decoder_ctx->height, AV_PIX_FMT_YUV420P,
        SWS_BILINEAR, nullptr, nullptr, nullptr);

    /* open the file to dump raw data */
    output_file = fopen(argv[3], "w+");

    /* actual decoding and dump the raw data */
    while (ret >= 0) {
        if ((ret = av_read_frame(input_ctx, &packet)) < 0)
            break;

        if (video_stream == packet.stream_index)
            ret = decode_write(decoder_ctx, &packet);

        av_packet_unref(&packet);
    }

    /* flush the decoder */
    packet.data = NULL;
    packet.size = 0;
    ret = decode_write(decoder_ctx, &packet);
    av_packet_unref(&packet);

    if (output_file)
        fclose(output_file);
    avcodec_free_context(&decoder_ctx);
    avformat_close_input(&input_ctx);
    av_buffer_unref(&hw_device_ctx);
    std::chrono::duration<double> time_duration = std::chrono::steady_clock::now() - time_base;
    std::cout << "time elapsed " << time_duration.count() << std::endl;
    return 0;
}