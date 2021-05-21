#include "helpers.h"
#include "assert.h"
#include "stdio.h"
#include "string.h"

#define assertm(exp, msg) assert(((void)msg, exp))

void print_error(int errcode)
{
    char serror[64] = {0};
    av_make_error_string(serror, 64, errcode);
    serror[strlen(serror)] = '\n';
    fprintf(stderr, serror + '\0');
}

void ffmpeg_call_i(int result)
{
    char serror[64] = {0};
    av_make_error_string(serror, 64, result);
    serror[strlen(serror)] = '\n';
    serror[strlen(serror)] = '\0';

    assertm(result >= 0, serror);
}

void ffmpeg_call_v(void* result_ptr)
{
    assertm(result_ptr, "Pointer is null");
}

void ffmpeg_call_im(int result, const char* error_msg)
{
    char serror[64] = {0};
    char message[256] = {0};
    av_make_error_string(serror, 64, result);
    serror[strlen(serror)] = '\n';
    serror[strlen(serror)] = '\0';
    size_t outsize = strlen(error_msg) + strlen(serror);
    
    #ifdef _WIN32
    sprintf_s(message, outsize, "Error: %s. %s", serror, error_msg);
    #else
    snprintf(message, outsize, "Error: %s. %s", serror, error_msg);
    #endif
    assertm(result >= 0, message);
}

void ffmpeg_call_vm(void* result_ptr, const char* error_msg)
{
    assertm(result_ptr, error_msg);
}

bool realloc_frame(AVFrame** av_frame)
{
    if(av_frame)
    {
        //reallocating frame
        av_frame_free(av_frame);
        if (!(*av_frame = av_frame_alloc()))
        {
            fprintf(stderr, "Can not alloc frame\n");
            return false;
        }
    }
    else
    {
        if (!(*av_frame = av_frame_alloc()))
        {
            fprintf(stderr, "Can not alloc frame\n");
            return false;
        }
    }
    return false;
}

enum AVPixelFormat correct_for_deprecated_pixel_format(enum AVPixelFormat pix_fmt)
{
    // Fix swscaler deprecated pixel format warning
    // (YUVJ has been deprecated, change pixel format to regular YUV)
    switch (pix_fmt)
    {
    case AV_PIX_FMT_YUVJ420P:
        return AV_PIX_FMT_YUV420P;
    case AV_PIX_FMT_YUVJ422P:
        return AV_PIX_FMT_YUV422P;
    case AV_PIX_FMT_YUVJ444P:
        return AV_PIX_FMT_YUV444P;
    case AV_PIX_FMT_YUVJ440P:
        return AV_PIX_FMT_YUV440P;
    default:
        return pix_fmt;
    }
}