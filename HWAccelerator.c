#include "HWAccelerator.h"
#include "helpers.h"

#include <libswscale/swscale.h>

enum AVPixelFormat hw_pix_fmt = AV_PIX_FMT_NONE;

enum AVPixelFormat get_hw_format(AVCodecContext *av_codec_ctx, const enum AVPixelFormat *pix_fmts)
{
    const enum AVPixelFormat *p;

    for (p = pix_fmts; *p != -1; p++)
    {
        if (*p == hw_pix_fmt)
            return *p;
    }

    fprintf(stderr, "Failed to get HW surface format.\n");
    return AV_PIX_FMT_NONE;
}

CHardwareAccelerator* hw_alloc()
{
    CHardwareAccelerator* hwdec = NULL;
    hwdec = (CHardwareAccelerator*)malloc(sizeof(CHardwareAccelerator));

    hwdec->sw_frame = NULL;
    hwdec->hw_device_ctx = NULL;

    return hwdec;
}

bool hw_select_device_manuality(CHardwareAccelerator** hwdec_ptr, AVCodec* av_codec, const char* device_name)
{
    CHardwareAccelerator* hwdec = *hwdec_ptr;

    //vaapi|vdpau|cuvid|dxva2|d3d11va|qvs|videotoolbox|drm|opencl|mediacodec|vulkan
    enum AVHWDeviceType devType = av_hwdevice_find_type_by_name(device_name);

    for (int i = 0;; i++)
    {
        const AVCodecHWConfig *config = avcodec_get_hw_config(av_codec, i);
        if (!config)
        {
            fprintf(stderr, "Decoder %s does not support device type %s.\n",
                    av_codec->name, av_hwdevice_get_type_name(devType));
            return false;
        }

        if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX && config->device_type == devType)
        {
            hw_pix_fmt = config->pix_fmt;
            break;
        }
    }

    hwdec->hw_device_type = devType;
    ffmpeg_call((void*)(
        hwdec->hw_device_ctx = av_hwdevice_ctx_alloc(devType)
    ));
    
    return true;
}

bool hw_select_device_automatically(CHardwareAccelerator** hwdec_ptr, AVCodec* av_codec)
{
    CHardwareAccelerator* hwdec = *hwdec_ptr;

    const AVCodecHWConfig *config = avcodec_get_hw_config(av_codec, 0);
    if(config && config->methods && AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX)
    {
        hw_pix_fmt = config->pix_fmt;
        hwdec->hw_device_type = config->device_type;
        ffmpeg_call((void*)(
            hwdec->hw_device_ctx = av_hwdevice_ctx_alloc(config->device_type)
        ));
        return true;
    }

    fprintf(stderr, "Cannot automatically detect hardware device for %s codec.\n", av_codec->name);
    return false;
}

bool hw_initialize_decoder(CHardwareAccelerator** hwdec_ptr, AVCodecContext** av_codec_ctx)
{
    CHardwareAccelerator* hwdec = *hwdec_ptr;

    (*av_codec_ctx)->get_format  = get_hw_format;

    //Initialize hardware context
    ffmpeg_call_m(
        av_hwdevice_ctx_create(&hwdec->hw_device_ctx, hwdec->hw_device_type, NULL, NULL, 0),
        "Failed to create specified HW device.\n"
    );

    ffmpeg_call((void*)(
        (*av_codec_ctx)->hw_device_ctx = av_buffer_ref(hwdec->hw_device_ctx)
    ));

    return true;
}

bool hw_initialize_encoder(CHardwareAccelerator** hwdec_ptr, AVCodec* av_codec, AVCodecContext** av_codec_ctx, AVCodecParameters* av_codec_params)
{

}

bool hw_get_decoded_frame(CHardwareAccelerator** hwdec_ptr, AVPacket* av_packet, AVFrame** av_frame)
{
    int ret = 0;
    CHardwareAccelerator* hwdec = *hwdec_ptr;

    if ((*av_frame)->format == hw_pix_fmt)
    {
        ffmpeg_call_m(
            av_hwframe_transfer_data(hwdec->sw_frame, *av_frame, 0),
            "Error transferring the data to system memory\n"
            );
        
        memcpy((*av_frame)->data, hwdec->sw_frame->data, sizeof(hwdec->sw_frame->data));
        memcpy((*av_frame)->linesize, hwdec->sw_frame->linesize, sizeof(hwdec->sw_frame->linesize));
        (*av_frame)->format = hwdec->sw_frame->format;
        return true;
    }
    return false;
}

bool hw_encode(CHardwareAccelerator** hwdec_ptr, AVPacket* av_packet, AVFrame** av_frame)
{

}

bool hw_close(CHardwareAccelerator** hwdec_ptr)
{
    av_frame_free(&(*hwdec_ptr)->sw_frame);
    av_buffer_unref(&(*hwdec_ptr)->hw_device_ctx);
    free(*hwdec_ptr);
}