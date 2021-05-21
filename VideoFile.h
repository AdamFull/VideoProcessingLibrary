#ifndef AV_VIDEOFILE
#define AV_VIDEOFILE

#include "DataStream.h"

/**
 * Structure for working with a video file.
 * 
 * Stores information about the video and audio stream. 
 * It also stores a pointer to an open file and data from the decoder.
 */
typedef struct CVideoFile
{

    AVFormatContext* av_format_ctx;
    AVPacket* av_packet;

    bool hwdecoding_video;
    bool hwdecoding_audio;

    CDataStream* vstream;
    CDataStream* astream;

} CVideoFile;

/**
 * Allocate an CVideoFile and set its fields to default values.
 *
 * @return An CVideoFile filled with default values or NULL on failure.
 */
CVideoFile* video_file_alloc(void);

/**
 * Initialize an CHardwareAccelerator as encoder.
 *
 * @param hwdec_ptr
 *
 * @return Returns true if all initialization got well.
 */
bool video_file_open_decode(CVideoFile**, const char*);

/**
 * Initialize an CHardwareAccelerator as encoder.
 *
 * @param hwdec_ptr
 *
 * @return Returns true if all initialization got well.
 */
bool video_file_read_frame(CVideoFile**);

/**
 * Initialize an CHardwareAccelerator as encoder.
 *
 * @param hwdec_ptr
 *
 * @return Returns true if all initialization got well.
 */
bool video_file_allow_hwdecoding_video(CVideoFile**);

/**
 * Initialize an CHardwareAccelerator as encoder.
 *
 * @param hwdec_ptr
 *
 * @return Returns true if all initialization got well.
 */
bool video_file_allow_hwdecoding_audio(CVideoFile**);

/**
 * Initialize an CHardwareAccelerator as encoder.
 *
 * @param hwdec_ptr
 *
 * @return Returns true if all initialization got well.
 */
void video_file_close(CVideoFile**);

#endif