#include <stdio.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
#include <libavutil/hwcontext.h>
#include <libavutil/opt.h>
#include <libavutil/avassert.h>
#include <unistd.h>

static AVBufferRef *hw_device_ctx = NULL;
static enum AVPixelFormat hw_pix_fmt;

static void pgm_save(unsigned char *buf, int wrap, int xsize, int ysize,
                     const char *filename) {
    FILE *f;
    int i;

    f = fopen(filename, "wb");
    fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);
    for (i = 0; i < ysize; i++)
        fwrite(buf + i * wrap, 1, xsize, f);
    fclose(f);
}

static int hw_decoder_init(AVCodecContext *ctx, const enum AVHWDeviceType type)
{
    int err = 0;

    if ((err = av_hwdevice_ctx_create(&hw_device_ctx, type,
                                      NULL, NULL, 0)) < 0) {
//        fprintf(stderr, "Failed to create specified HW device.\n");
//        printf("err: %d\n",err);

        return err;
    }
    ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);

    return err;
}


static enum AVPixelFormat get_hw_format(AVCodecContext *ctx,
                                        const enum AVPixelFormat *pix_fmts) {
    const enum AVPixelFormat *p;

    for (p = pix_fmts; *p != -1; p++) {
        if (*p == hw_pix_fmt)
            return *p;
    }

    //fprintf(stderr, "Failed to get HW surface format.\n");
    return AV_PIX_FMT_NONE;
}


static int compare_frames(AVFrame *frame1, AVFrame *frame2) {
    int code = 0;
    if (frame1->width != frame2->width) {
        code |= 1;
    }
    if (frame1->height != frame2->height) {
        code |= 2;
    }
    if (code) {
        // Frames have different dimensions
        fprintf(stderr,
                "Frames have different dimensions:\tframe1->width: %d\tframe2->width: %d\tframe1->height: %d\tframe2->height: %d\n",
                frame1->width, frame2->width, frame1->height, frame2->height);

        return code;
    }

    for (int i = 0; i < AV_NUM_DATA_POINTERS && frame1->buf[i]; i++) {
        uint8_t *ptr1 = frame1->data[i];
        uint8_t *ptr2 = frame2->data[i];
//        ptr1[0] = ~ptr1[0];
        for (int y = 0; y < frame1->height; y++) {
            if (memcmp(ptr1, ptr2, frame1->width)) {
                printf("differed on data line %d (%d, %d)\n", i, frame1->linesize[i], frame2->linesize[i]);
                return 4;
            }
            ptr1 += frame1->linesize[i];
            ptr2 += frame2->linesize[i];
        }
    }

    return 0;
}

static int decode_write(AVCodecContext *avctx, AVPacket *packet, AVCodecContext *sw_avctx, AVPacket *sw_packet) {
    AVFrame *frame = NULL, *sw_frame = NULL, *real_sw_frame = NULL;
    AVFrame *tmp_frame = NULL;
    int ret = 0;

    ret = avcodec_send_packet(avctx, packet);
    if (ret < 0) {
        //fprintf(stderr, "Error during decoding\n");
        return ret;
    }
    ret = avcodec_send_packet(sw_avctx, sw_packet);
    if (ret < 0) {
        //fprintf(stderr, "Error during decoding\n");
        return ret;
    }

    while (1) {
        if (!(frame = av_frame_alloc()) || !(sw_frame = av_frame_alloc()) || !(real_sw_frame = av_frame_alloc())) {
            //fprintf(stderr, "Can not alloc frame\n");
            abort();
        }

        ret = avcodec_receive_frame(avctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            av_frame_free(&frame);
            av_frame_free(&sw_frame);
            av_frame_free(&real_sw_frame);
            return 0;
        } else if (ret < 0) {
            //fprintf(stderr, "Error while decoding\n");
            goto fail;
        }
        ret = avcodec_receive_frame(sw_avctx, real_sw_frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            abort();
        } else if (ret < 0) {
            //fprintf(stderr, "Error while decoding\n");
            abort();
        }

        if (frame->format == hw_pix_fmt) {
            
/* retrieve data from GPU to CPU */
            if ((ret = av_hwframe_transfer_data(sw_frame, frame, 0)) < 0) {
                //fprintf(stderr, "Error transferring the data to system memory\n");
                goto fail;
            }
            tmp_frame = sw_frame;
        } else
            tmp_frame = frame;

        ret = compare_frames(tmp_frame, real_sw_frame);
        if (ret) {
            pgm_save(tmp_frame->data[0], tmp_frame->linesize[0], tmp_frame->width, tmp_frame->height, "hw_frame.pgm");
            pgm_save(real_sw_frame->data[0], real_sw_frame->linesize[0],real_sw_frame->width, real_sw_frame->height, "sw_frame.pgm");

            fprintf(stderr, "HW/SW buffer contents differ; see {hw,sw}_frame.pgm\n");
            abort();
            _exit(ret + 1);
        }
        fprintf(stderr, "\t --> Decoding completed correctly\n");


    fail:
        av_frame_free(&frame);
        av_frame_free(&sw_frame);
        av_frame_free(&real_sw_frame);
        if (ret < 0)
            return ret;
    }
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    av_log_set_level(AV_LOG_PANIC);

    int keep = 0;
    int i;

    AVFormatContext *input_ctx = NULL;
    int video_stream, ret;
    AVStream *video = NULL;
    AVCodecContext *decoder_ctx = NULL;
    const AVCodec *decoder = NULL;
    AVPacket *packet = NULL;
    enum AVHWDeviceType type;

    AVFormatContext *sw_input_ctx = NULL;
    int sw_video_stream;
    AVStream *sw_video = NULL;
    AVCodecContext *sw_decoder_ctx = NULL;
    const AVCodec *sw_decoder = NULL;
    AVPacket *sw_packet = NULL;

    type = av_hwdevice_find_type_by_name("vaapi");
    if (type == AV_HWDEVICE_TYPE_NONE) {
        abort();
    }

    packet = av_packet_alloc();
    if (!packet) {
        //fprintf(stderr, "Failed to allocate AVPacket\n");
        abort();
    }
    sw_packet = av_packet_alloc();
    if (!sw_packet) {
        //fprintf(stderr, "Failed to allocate AVPacket\n");
        abort();
    }

    input_ctx = avformat_alloc_context();
    if (!input_ctx) {
        printf("FAILED avformat_alloc_context\n");
        abort();
    }
    sw_input_ctx = avformat_alloc_context();
    if (!sw_input_ctx) {
        printf("FAILED avformat_alloc_context\n");
        abort();
    }

    // Allocate an AVIOContext for the input buffer
    unsigned char *tmp = NULL;
    tmp = malloc(size + AV_INPUT_BUFFER_PADDING_SIZE);
    memcpy(tmp, data, size);
    memset(tmp + size, 0, AV_INPUT_BUFFER_PADDING_SIZE);
    unsigned char *sw_tmp = NULL;
    sw_tmp = malloc(size + AV_INPUT_BUFFER_PADDING_SIZE);
    memcpy(sw_tmp, data, size);
    memset(sw_tmp + size, 0, AV_INPUT_BUFFER_PADDING_SIZE);

    AVIOContext *ioContext = avio_alloc_context(tmp, size, 0, NULL, NULL, NULL, NULL);
    if (!ioContext) {
        // Handle error
        printf("FAILED avio_alloc_context\n");
        goto io_context_failed;
    }
    input_ctx->pb = ioContext;
    AVIOContext *sw_ioContext = avio_alloc_context(sw_tmp, size, 0, NULL, NULL, NULL, NULL);
    if (!sw_ioContext) {
        // Handle error
        printf("FAILED avio_alloc_context\n");
        goto io_context_failed;
    }
    sw_input_ctx->pb = sw_ioContext;

    if (avformat_open_input(&input_ctx, NULL, NULL, NULL) != 0) {
        goto input_failed;
    }
    if (avformat_open_input(&sw_input_ctx, NULL, NULL, NULL) != 0) {
        goto input_failed;
    }

    if (avformat_find_stream_info(input_ctx, NULL) < 0) {
        //fprintf(stderr, "Cannot find input stream information.\n");
        goto failed;
    }
    if (avformat_find_stream_info(sw_input_ctx, NULL) < 0) {
        //fprintf(stderr, "Cannot find input stream information.\n");
        goto failed;
    }


/* find the video stream information */

    ret = av_find_best_stream(input_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
    if (ret < 0) {
        //fprintf(stderr, "Cannot find a video stream in the input file\n");
        goto failed;
    }
    video_stream = ret;
    ret = av_find_best_stream(sw_input_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &sw_decoder, 0);
    if (ret < 0) {
        //fprintf(stderr, "Cannot find a video stream in the input file\n");
        goto failed;
    }
    sw_video_stream = ret;

    for (i = 0;; i++) {
        const AVCodecHWConfig *config = avcodec_get_hw_config(decoder, i);
        if (!config) {
            //fprintf(stderr, "Decoder %s does not support device type %s.\n",
//                    decoder->name, av_hwdevice_get_type_name(type));
            keep = -1;
            goto failed;
        }
        if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
            config->device_type == type) {
            hw_pix_fmt = config->pix_fmt;
            break;
        }
    }

    if (!(decoder_ctx = avcodec_alloc_context3(decoder)))
        abort();
    if (!(sw_decoder_ctx = avcodec_alloc_context3(sw_decoder)))
        abort();

    video = input_ctx->streams[video_stream];
    if (avcodec_parameters_to_context(decoder_ctx, video->codecpar) < 0)
        abort();
    sw_video = sw_input_ctx->streams[sw_video_stream];
    if (avcodec_parameters_to_context(sw_decoder_ctx, sw_video->codecpar) < 0)
        abort();

    decoder_ctx->get_format = get_hw_format;

    if (hw_decoder_init(decoder_ctx, type) < 0) {
//        fprintf(stderr, "NOT CONFIGURED\n");
        abort();
    }

    if ((ret = avcodec_open2(decoder_ctx, decoder, NULL)) < 0) {
        //fprintf(stderr, "Failed to open codec for stream #%u\n", video_stream);
        abort();
    }
    if ((ret = avcodec_open2(sw_decoder_ctx, sw_decoder, NULL)) < 0) {
        //fprintf(stderr, "Failed to open codec for stream #%u\n", video_stream);
        abort();
    }


/* actual decoding and dump the raw data */

    while (ret >= 0) {
        if ((ret = av_read_frame(input_ctx, packet)) < 0)
            break;
        if ((ret = av_read_frame(sw_input_ctx, sw_packet)) < 0)
            break;

        if (video_stream == packet->stream_index) {
            if (sw_video_stream != sw_packet->stream_index) {
                abort();
            }
            ret = decode_write(decoder_ctx, packet, sw_decoder_ctx, sw_packet);
        }

        av_packet_unref(packet);
        av_packet_unref(sw_packet);
    }

    failed:
    avformat_close_input(&input_ctx);
    avformat_close_input(&sw_input_ctx);

    input_failed:
    avio_close(ioContext);
    avformat_free_context(input_ctx);
    avio_close(sw_ioContext);
    avformat_free_context(sw_input_ctx);

    io_context_failed:
    av_packet_free(&packet);
    avcodec_free_context(&decoder_ctx);
    av_buffer_unref(&hw_device_ctx);

    av_packet_free(&sw_packet);
    avcodec_free_context(&sw_decoder_ctx);

    return keep;
}
