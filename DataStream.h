#ifndef AV_DATASTREAM
#define AV_DATASTREAM

#include "HWAccelerator.h"

typedef bool (*data_stream_get_sw_data_t)(struct CDataStream**);

/**
 * The main structure in the presented api.
 * 
 * It is designed to store basic variables for easy interaction with the audio / video stream. 
 * The structure stores data for encoding / decoding the data stream. Initially, the library 
 * was intended for real-time video decoding and converting frames to RGB format for further 
 * display on the screen.
 */
typedef struct CDataStream
{
    /**
     * Pointer to the hardware accelerator context.
     */
    CHardwareAccelerator*   hwdecoder;

    char*                   manuality_device_name;

    bool                    is_hardware_avaliable;

    int32_t                 thread_count;

    int32_t                 thread_type;

    /**
     * 
     */
    enum AVMediaType        stream_type;

    /**
     * Source frame size and converted frame size.
     */
    int32_t                 fwidth, fheight, swidth, sheight;

    /**
     * Is there a need to update the scaler. 
     * It is necessary if the data on the output size has been changed during decoding.
     */
    bool                    vneed_rescaler_update;

    /**
     * Whether hardware frame decoding is enabled.
     */
    bool                    allow_hardware_decoding;

    /**
     * Rational number (pair of numerator and denominator).
     * Describes a place in a stream.
     */
    AVRational              time_base;

    /**
     * The size of the allocated memory block for a frame, taking into account alignment.
     */
    int32_t                 allocated_block_size;

    /**
     * Buffer containing the aligned frame data.
     */
    uint8_t*                block_buffer;

    int64_t                 pts;

    int32_t                 data_stream_index;

    AVPacket*               av_first_pkt;

    AVCodecContext*         av_codec_ctx;

    enum AVPixelFormat      av_output_pix_fmt;
    int                     av_output_flags;

    /**
     * This structure describes decoded (raw) audio or video data.
     */
    AVFrame*                av_frame;

    /**
     * This structure contains rescaled frame data.
     */
    AVFrame*                sc_frame;

    struct SwsContext*      sws_scaler_ctx;
    struct SwrContext*      swr_ctx;

    /**
     * Contains pointer to rescaler.
     */
    data_stream_get_sw_data_t data_stream_get_sw_data_ptr;

    FILE* file_writer;

}CDataStream;

/**
 * Allocate an CDataStream and set its fields to default values.
 *
 * @return An CDataStream filled with default values or NULL on failure.
 */
CDataStream* data_stream_alloc(void);

/**
 * Initializing a CDataStream structure to decode a data stream.
 *
 * @param stream_ptr Pointer to pointer to CDataStream structure.
 *
 * @param av_format_ctx Pointer to AVFormatContext codec context.
 *
 * @param stream_type Choosing a data stream type.
 *
 * @param allow_hardware Allow using gpu for decoding frames.
 *
 * @return Returns true if initialization was successful.
 */
bool data_stream_initialize_decode(CDataStream** stream_ptr, AVFormatContext* av_format_ctx, enum AVMediaType stream_type, bool allow_hardware);

/**
 * Initializing a CDataStream structure to encode a data stream.
 *
 * @param stream_ptr Pointer to pointer to CDataStream structure.
 *
 * @param filename Path to output file.
 *
 * @param stream_type Choosing a data stream type.
 *
 * @param allow_hardware Allow using gpu for decoding frames.
 *
 * @return Returns true if initialization was successful.
 */
bool data_stream_initialize_encode(CDataStream** stream_ptr, const char* filename, enum AVCodecID id, enum AVMediaType stream_type, bool allow_hardware);

/**
 * Sends data to a decoder for further decoding in order to obtain an image.
 *
 * @param stream_ptr Pointer to pointer to CDataStream structure.
 *
 * @param av_format_ctx Format I/O context.
 *
 * @param av_packet This structure stores compressed data.
 *
 * @return Returns true if initialization was successful.
 */
int data_stream_decode(CDataStream** stream_ptr, AVFormatContext* av_format_ctx, AVPacket* av_packet);

/**
 * Method to get the number of seconds from the first decoded or encoded frame.
 *
 * @param stream_ptr
 *
 * @return Returns the time elapsed from the beginning of the stream in seconds.
 */
double data_stream_get_pt_seconds(CDataStream** stream_ptr);

void data_stream_set_hw_device_manuality(CDataStream** stream_ptr, const char* device_name);

void data_stream_set_thread_settings(CDataStream** stream_ptr, int32_t thread_count, int32_t thread_type_flags);

/**
 * Initialize an CHardwareAccelerator as encoder.
 *
 * @param stream_ptr
 *
 * @return Returns true if all initialization got well.
 */
void data_stream_set_frame_size(CDataStream** stream_ptr, int32_t nwidth, int32_t nheight);

/**
 * Callback called if an audio sample has been decoded or encoded, and writes it to an aligned buffer: block_buffer.
 *
 * @param stream_ptr
 *
 * @return Returns true if all initialization got well.
 */
bool data_stream_get_sw_data_audio(CDataStream** stream_ptr);

/**
 * Callback called if a video frame has been decoded or encoded, and writes it to an aligned buffer: block_buffer.
 *
 * @param stream_ptr
 *
 * @return Returns true if all initialization got well.
 */
bool data_stream_get_sw_data_video(CDataStream** stream_ptr);

/**
 * Closes the stream and clears all memory allocated for it.
 *
 * @param stream_ptr
 *
 * @return Returns true.
 */
void data_stream_close(CDataStream** stream_ptr);


#endif