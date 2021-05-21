cmake_minimum_required(VERSION 2.8)
project(EVPL C)

set(CMAKE_C_STANDARD 11)

file(GLOB VIDEO_ENCODER_DECODER_FILES *.c *.h)

if(WIN32)
    set(SYSTEM_TYPE windows)
    set(ARCHITECTURE ${CMAKE_HOST_SYSTEM_PROCESSOR})
elseif(UNIX AND NOT APPLE)
if(ANDROID)
    set(SYSTEM_TYPE android)
    set(ARCHITECTURE ${ANDROID_ABI})
else()
    set(SYSTEM_TYPE linux)
    set(ARCHITECTURE ${CMAKE_HOST_SYSTEM_PROCESSOR})
elseif(UNIX AND APPLE)
    set(SYSTEM_TYPE apple)
    set(ARCHITECTURE ${CMAKE_HOST_SYSTEM_PROCESSOR})
endif()

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        set(CURRENT_BUILDTYPE "debug")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DVENC_DEBUG")
    elseif(${CMAKE_BUILD_TYPE} STREQUAL "Release")
        set(CURRENT_BUILDTYPE "release")
    endif()

if(DEFINED FF_LOCAL_LIBRARIES)
    # Configuring build if using local libraries
	set(FFMPEG_LIB_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg)

    set(FFMPEG_LIB_PLATFORMSPEC_PATH ${FFMPEG_LIB_PREFIX}/${SYSTEM_TYPE}/${ARCHITECTURE})

    set(FFMPEG_LIBRARIES 
            ${FFMPEG_LIB_PLATFORMSPEC_PATH}/libavcodec.so.58 
            ${FFMPEG_LIB_PLATFORMSPEC_PATH}/libavdevice.so.58 
            ${FFMPEG_LIB_PLATFORMSPEC_PATH}/libavfilter.so.7 
            ${FFMPEG_LIB_PLATFORMSPEC_PATH}/libavformat.so.58
            ${FFMPEG_LIB_PLATFORMSPEC_PATH}/libavutil.so.56
            ${FFMPEG_LIB_PLATFORMSPEC_PATH}/libpostproc.so.55
            ${FFMPEG_LIB_PLATFORMSPEC_PATH}/libswresample.so.3
            ${FFMPEG_LIB_PLATFORMSPEC_PATH}/libswscale.so.5
        )

    include_directories(${FFMPEG_LIB_PREFIX}/include)
elseif(DEFINED FF_VITABUILD)
    # Configuring build for ps vita
    if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
        if(DEFINED ENV{VITASDK})
            set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
        else()
            message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
        endif()
    endif()

    include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

    set(FFMPEG_LIBRARIES 
        libavdevice
        libavfilter
        libavformat
        libavcodec
        libswresample
        libswscale
        libavutil
    )

    set(SYSTEM_TYPE psvita)
    set(ARCHITECTURE arm64)

else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(FFMPEG_LIBS REQUIRED IMPORTED_TARGET
        libavdevice
        libavfilter
        libavformat
        libavcodec
        libswresample
        libswscale
        libavutil
    )
    set(FFMPEG_LIBRARIES PkgConfig::FFMPEG_LIBS)
endif()

message("Configuring " ${CURRENT_BUILDTYPE} " build for system " ${SYSTEM_TYPE}:${ARCHITECTURE})


if(DEFINED COMPILE_STATIC OR DEFINED FF_VITABUILD)
    add_library(${PROJECT_NAME} STATIC ${VIDEO_ENCODER_DECODER_FILES})
else()
    add_library(${PROJECT_NAME} STATIC ${VIDEO_ENCODER_DECODER_FILES})
endif()

target_link_libraries(${PROJECT_NAME} ${FFMPEG_LIBRARIES})
