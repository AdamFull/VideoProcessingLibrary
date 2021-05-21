#include "DataStream.h"
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/imgutils.h>
#include <string.h>
#include "helpers.h"

CDataStream* data_stream_alloc()
{
    CDataStream* dstream = NULL;
    dstream = (CDataStream*)malloc(sizeof(CDataStream));
    dstream->hwdecoder = hw_alloc();

    dstream->manuality_device_name = NULL;
    dstream->is_hardware_avaliable = false;
    dstream->thread_count = 1;
    dstream->thread_type |= FF_THREAD_FRAME | FF_THREAD_SLICE;
    dstream->stream_type = 0;
    dstream->vneed_rescaler_update = 0;
    dstream->allow_hardware_decoding = 0;
    dstream->allocated_block_size = 0;
    dstream->block_buffer = NULL;
    dstream->pts = 0;
    dstream->data_stream_index = -1;
    dstream->av_codec_ctx = NULL;
    dstream->av_output_pix_fmt = AV_PIX_FMT_RGB0;
    dstream->av_output_flags = SWS_BICUBLIN;
    dstream->av_frame = NULL;
    dstream->sc_frame = NULL;
    dstream->av_first_pkt = NULL;
    dstream->sws_scaler_ctx = NULL;
    dstream->swr_ctx = NULL;

    return dstream;
}

bool data_stream_initialize_decode(CDataStream** stream_ptr, AVFormatContext* av_format_ctx, enum AVMediaType stream_type, bool allow_hardware)
{
    CDataStream* stream = *stream_ptr;
    AVCodecParameters *av_codec_params;
    AVCodec *av_codec;
    bool hw_result = false;

    stream->stream_type = stream_type;
    stream->allow_hardware_decoding = allow_hardware;

    (*stream_ptr)->stream_type = stream_type;
    (*stream_ptr)->allow_hardware_decoding = allow_hardware;

    // Find the first valid video stream inside the file
    ffmpeg_call((stream->data_stream_index = av_find_best_stream(av_format_ctx, stream_type, -1, -1, &av_codec, 0)));

    av_codec_params = av_format_ctx->streams[stream->data_stream_index]->codecpar;
    stream->time_base = av_format_ctx->streams[stream->data_stream_index]->time_base;
    if(stream_type == AVMEDIA_TYPE_VIDEO)
    {
        stream->fwidth = av_codec_params->width;
        stream->fheight = av_codec_params->height;
    }

    if(allow_hardware)
    {
        if(stream->manuality_device_name)
            hw_result = hw_select_device_manuality(&stream->hwdecoder, av_codec, stream->manuality_device_name);
        else
            hw_result = hw_select_device_automatically(&stream->hwdecoder, av_codec);
    }

    // Set up a codec context for the decoder
    stream->av_codec_ctx = avcodec_alloc_context3(av_codec);
    if (!stream->av_codec_ctx)
    {
        printf("Couldn't create AVCodecContext\n");
        return false;
    }

    if (avcodec_parameters_to_context(stream->av_codec_ctx, av_codec_params) < 0)
    {
        printf("Couldn't initialize AVCodecContext\n");
        return false;
    }

    if(hw_result)
        stream->is_hardware_avaliable = hw_initialize_decoder(&stream->hwdecoder, &stream->av_codec_ctx);
    
    stream->av_codec_ctx->thread_count = stream->thread_count;
    stream->av_codec_ctx->thread_type |= stream->thread_type;

    ffmpeg_call_m(avcodec_open2(stream->av_codec_ctx, av_codec, NULL), "Couldn't open codec\n");

    ffmpeg_call_m((void*)(
        stream->sc_frame = av_frame_alloc()), "Can not alloc frame\n"
        );

    if(stream_type == AVMEDIA_TYPE_AUDIO)
        stream->data_stream_get_sw_data_ptr = data_stream_get_sw_data_audio;
    else if(stream_type == AVMEDIA_TYPE_VIDEO)
        stream->data_stream_get_sw_data_ptr = data_stream_get_sw_data_video;

    return true;

}

bool data_stream_initialize_encode(CDataStream** stream_ptr, const char* filename, enum AVCodecID id, enum AVMediaType stream_type, bool allow_hardware)
{
    CDataStream* stream = *stream_ptr;
    AVCodec* av_codec = NULL;
    int result;

    ffmpeg_call_m((void*)(
        av_codec = avcodec_find_encoder(id)), "Codec could not found.\n"
        );
    
    ffmpeg_call_m((void*)(
        stream->av_codec_ctx = avcodec_alloc_context3(av_codec)), "Cannot allocate codec context.\n"
        );

    //Init codec params

    //Init codec params

    if (av_codec->id == AV_CODEC_ID_H264)
        ffmpeg_call(av_opt_set(stream->av_codec_ctx->priv_data, "preset", "slow", 0));

    ffmpeg_call((result = avcodec_open2(stream->av_codec_ctx, av_codec, NULL)));

    #ifdef _WIN32
    if((result = fopen_s(stream->file_writer, filename, "wb")) < 0)
    #else
    if((stream->file_writer = fopen(filename, "wb")) < 0)
    #endif
    {
        fprintf(stderr, "Cannot open file writer.\n");
        return false;
    }

    return true;
}

int data_stream_decode(CDataStream** stream_ptr, AVFormatContext* av_format_ctx, AVPacket* av_packet)
{
    int response;
    CDataStream* stream = *stream_ptr;

    ffmpeg_call((
        response = avcodec_send_packet(stream->av_codec_ctx, av_packet)
    ));

    while(true)
    {
        if (!(stream->av_frame = av_frame_alloc()) || !(stream->hwdecoder->sw_frame = av_frame_alloc())) 
        {
            fprintf(stderr, "Can not alloc frames\n");
            response = AVERROR(ENOMEM);
            print_error(response);
            goto fail;
        }

        response = avcodec_receive_frame(stream->av_codec_ctx, stream->av_frame);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
        {
            av_frame_free(&stream->av_frame);
            av_frame_free(&stream->hwdecoder->sw_frame);
            print_error(response);
            return 0;
        }
        else if (response < 0)
        {
            fprintf(stderr, "Error while decoding\n");
            print_error(response);
            goto fail;
        }

        if (stream->is_hardware_avaliable && stream->allow_hardware_decoding)
        {
            if (!hw_get_decoded_frame(&stream->hwdecoder, av_packet, &stream->av_frame))
            {
                printf("Failed while decoding frame on gpu.\n");
                goto fail;
            }
        }

        stream->pts = stream->av_frame->pts;

        if(!stream->data_stream_get_sw_data_ptr(stream_ptr))
        {
            printf("Failed while scaling frame.\n");
            return 0;
        }

        fail:
            av_frame_free(&stream->av_frame);
            av_frame_free(&stream->hwdecoder->sw_frame);
            if (response < 0)
            {
                print_error(response);
                return response;
            }
    }

    return 0;
}

bool data_stream_get_sw_data_audio(CDataStream** stream_ptr)
{
    CDataStream* stream = *stream_ptr;

    ffmpeg_call(
        av_samples_alloc(&stream->block_buffer, &stream->allocated_block_size, stream->av_frame->channels, stream->av_frame->sample_rate, AV_SAMPLE_FMT_FLT, 0)
    );

    if (!stream->swr_ctx)
    {
        ffmpeg_call((void*)(
        stream->swr_ctx = swr_alloc_set_opts(NULL, stream->av_codec_ctx->channel_layout,
                                             AV_SAMPLE_FMT_S16, stream->av_codec_ctx->sample_rate,
                                             stream->av_codec_ctx->channel_layout, stream->av_codec_ctx->sample_fmt,
                                             stream->av_codec_ctx->sample_rate, 0, NULL)
        ));

        ffmpeg_call_m(
            swr_init(stream->swr_ctx), 
            "Couldn't initialize swr context\n"
            );
    }

    int response = 0;
    ffmpeg_call_m((
        response = swr_convert(stream->swr_ctx, &stream->block_buffer, stream->av_frame->sample_rate, (const uint8_t **)stream->av_frame->extended_data, stream->av_frame->nb_samples)),
        "Couldn't convert audio frame.\n"
    );
    ffmpeg_call((
        stream->allocated_block_size = av_get_bytes_per_sample(AV_SAMPLE_FMT_FLT) * stream->av_frame->channels * response
    ));

    return true;
}

bool data_stream_get_sw_data_video(CDataStream** stream_ptr)
{
    CDataStream* stream = *stream_ptr;

    if (!stream->sws_scaler_ctx || stream->vneed_rescaler_update)
    {
        if (stream->vneed_rescaler_update && stream->sws_scaler_ctx)
            sws_freeContext(stream->sws_scaler_ctx);

        enum AVPixelFormat source_pix_fmt = correct_for_deprecated_pixel_format((enum AVPixelFormat)stream->av_frame->format);
        //Send here frame params
        //BEST QUALITY/PERFOMANCE: SWS_BICUBLIN, SWS_AREA
        ffmpeg_call((void*)(
        stream->sws_scaler_ctx = sws_getContext(stream->fwidth, stream->fheight, source_pix_fmt,
                                                stream->swidth, stream->sheight, stream->av_output_pix_fmt,
                                                stream->av_output_flags, NULL, NULL, NULL)
        ));

        if (stream->vneed_rescaler_update && stream->block_buffer)
            av_free(stream->block_buffer);

        stream->allocated_block_size = av_image_get_buffer_size(stream->av_output_pix_fmt, stream->swidth, stream->sheight, 1);
        stream->block_buffer = (uint8_t *)av_malloc((stream->allocated_block_size) * sizeof(uint8_t));
        stream->vneed_rescaler_update = false;
    }

    if (!stream->sws_scaler_ctx)
    {
        printf("Couldn't create sws scaler\n");
        return false;
    }

    av_image_fill_arrays(stream->sc_frame->data, stream->sc_frame->linesize, stream->block_buffer, stream->av_output_pix_fmt, stream->swidth, stream->sheight, 1);
    ffmpeg_call(
        sws_scale(stream->sws_scaler_ctx, stream->av_frame->data, stream->av_frame->linesize, 0, stream->fheight, stream->sc_frame->data, stream->sc_frame->linesize)
    );
    return true;
}

double data_stream_get_pt_seconds(CDataStream** stream_ptr)
{
    int64_t pts = (*stream_ptr)->pts;
    AVRational time_base = (*stream_ptr)->time_base;

    return pts * (double)time_base.num / (double)time_base.den;
}

void data_stream_set_hw_device_manuality(CDataStream** stream_ptr, const char* device_name)
{
    CDataStream* stream = *stream_ptr;
    size_t str_size = strlen(device_name) + 1;
    stream->manuality_device_name = (char*)malloc(sizeof(char) * str_size);
    strncpy(stream->manuality_device_name, device_name, str_size);
}

void data_stream_set_thread_settings(CDataStream** stream_ptr, int32_t thread_count, int32_t thread_type_flags)
{
    CDataStream* stream = *stream_ptr;
    stream->thread_count = thread_count;
    stream->thread_type |= thread_type_flags;
}

void data_stream_set_frame_size(CDataStream** stream_ptr, int32_t nwidth, int32_t nheight)
{
    CDataStream* stream = *stream_ptr;

    if(nwidth != stream->swidth && nheight != stream->sheight)
    {
        stream->swidth = nwidth;
        stream->sheight = nheight;
        stream->vneed_rescaler_update = true;
    }
}

void data_stream_close(CDataStream** stream_ptr)
{
    CDataStream* stream = *stream_ptr;

    if(stream->manuality_device_name)
        free((void*)stream->manuality_device_name);

    hw_close(&stream->hwdecoder);
    av_free(stream->block_buffer);
    avcodec_free_context(&stream->av_codec_ctx);
    av_frame_free(&stream->av_frame);
    av_frame_free(&stream->sc_frame);
    sws_freeContext(stream->sws_scaler_ctx);
    swr_free(&stream->swr_ctx);
    free(stream);
}