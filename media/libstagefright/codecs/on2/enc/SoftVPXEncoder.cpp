/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// #define LOG_NDEBUG 0
#define LOG_TAG "SoftVPXEncoder"
#include "SoftVPXEncoder.h"

#include <utils/Log.h>

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MediaDefs.h>

namespace android {


template<class T>
static void InitOMXParams(T *params) {
    params->nSize = sizeof(T);
    // OMX IL 1.1.2
    params->nVersion.s.nVersionMajor = 1;
    params->nVersion.s.nVersionMinor = 1;
    params->nVersion.s.nRevision = 2;
    params->nVersion.s.nStep = 0;
}


static int GetCPUCoreCount() {
    int cpuCoreCount = 1;
#if defined(_SC_NPROCESSORS_ONLN)
    cpuCoreCount = sysconf(_SC_NPROCESSORS_ONLN);
#else
    // _SC_NPROC_ONLN must be defined...
    cpuCoreCount = sysconf(_SC_NPROC_ONLN);
#endif
    CHECK_GE(cpuCoreCount, 1);
    return cpuCoreCount;
}


// This color conversion utility is copied from SoftMPEG4Encoder.cpp
inline static void ConvertYUV420SemiPlanarToYUV420Planar(uint8_t *inyuv, uint8_t* outyuv,
                                                         int32_t width, int32_t height) {

    int32_t outYsize = width * height;
    uint32_t *outy =  (uint32_t *) outyuv;
    uint16_t *outcb = (uint16_t *) (outyuv + outYsize);
    uint16_t *outcr = (uint16_t *) (outyuv + outYsize + (outYsize >> 2));

    /* Y copying */
    memcpy(outy, inyuv, outYsize);

    /* U & V copying */
    uint32_t *inyuv_4 = (uint32_t *) (inyuv + outYsize);
    for (int32_t i = height >> 1; i > 0; --i) {
        for (int32_t j = width >> 2; j > 0; --j) {
            uint32_t temp = *inyuv_4++;
            uint32_t tempU = temp & 0xFF;
            tempU = tempU | ((temp >> 8) & 0xFF00);

            uint32_t tempV = (temp >> 8) & 0xFF;
            tempV = tempV | ((temp >> 16) & 0xFF00);

            // Flip U and V
            *outcb++ = tempV;
            *outcr++ = tempU;
        }
    }
}


SoftVPXEncoder::SoftVPXEncoder(const char *name,
                               const OMX_CALLBACKTYPE *callbacks,
                               OMX_PTR appData,
                               OMX_COMPONENTTYPE **component)
    : SimpleSoftOMXComponent(name, callbacks, appData, component),
      mCodecContext(NULL),
      mCodecConfiguration(NULL),
      mCodecInterface(NULL),
      mWidth(176),
      mHeight(144),
      mBitrate(192000),  // in bps
      mBitrateControlMode(VPX_VBR),  // variable bitrate
      mFrameDuration(33333),  // Defaults to 30 fps
      mInputBufferAlignment(1),
      mColorFormat(OMX_COLOR_FormatYUV420Planar),
      mConversionBuffer(NULL) {

    initPorts();
}


SoftVPXEncoder::~SoftVPXEncoder() {
    releaseEncoder();
}


void SoftVPXEncoder::initPorts() {
    OMX_PARAM_PORTDEFINITIONTYPE inputPort;
    OMX_PARAM_PORTDEFINITIONTYPE outputPort;

    InitOMXParams(&inputPort);
    InitOMXParams(&outputPort);

    inputPort.nBufferCountMin = kNumBuffers;
    inputPort.nBufferCountActual = inputPort.nBufferCountMin;
    inputPort.bEnabled = OMX_TRUE;
    inputPort.bPopulated = OMX_FALSE;
    inputPort.eDomain = OMX_PortDomainVideo;
    inputPort.bBuffersContiguous = OMX_FALSE;
    inputPort.format.video.pNativeRender = NULL;
    inputPort.format.video.nFrameWidth = mWidth;
    inputPort.format.video.nFrameHeight = mHeight;
    inputPort.format.video.nStride = inputPort.format.video.nFrameWidth;
    inputPort.format.video.nSliceHeight = inputPort.format.video.nFrameHeight;
    inputPort.format.video.nBitrate = 0;
    // frameRate is reciprocal of frameDuration, which is
    // in microseconds. It is also in Q16 format.
    inputPort.format.video.xFramerate = (mFrameDuration*1000000) << 16;
    inputPort.format.video.bFlagErrorConcealment = OMX_FALSE;
    inputPort.nPortIndex = 0;
    inputPort.eDir = OMX_DirInput;
    inputPort.nBufferAlignment = mInputBufferAlignment;
    inputPort.format.video.cMIMEType =
        const_cast<char *>(MEDIA_MIMETYPE_VIDEO_RAW);
    inputPort.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    inputPort.format.video.eColorFormat = mColorFormat;
    inputPort.format.video.pNativeWindow = NULL;
    inputPort.nBufferSize =
        (inputPort.format.video.nStride *
        inputPort.format.video.nSliceHeight * 3) / 2;

    addPort(inputPort);

    outputPort.nBufferCountMin = kNumBuffers;
    outputPort.nBufferCountActual = outputPort.nBufferCountMin;
    outputPort.bEnabled = OMX_TRUE;
    outputPort.bPopulated = OMX_FALSE;
    outputPort.eDomain = OMX_PortDomainVideo;
    outputPort.bBuffersContiguous = OMX_FALSE;
    outputPort.format.video.pNativeRender = NULL;
    outputPort.format.video.nFrameWidth = mWidth;
    outputPort.format.video.nFrameHeight = mHeight;
    outputPort.format.video.nStride = outputPort.format.video.nFrameWidth;
    outputPort.format.video.nSliceHeight = outputPort.format.video.nFrameHeight;
    outputPort.format.video.nBitrate = mBitrate;
    outputPort.format.video.xFramerate = 0;
    outputPort.format.video.bFlagErrorConcealment = OMX_FALSE;
    outputPort.nPortIndex = 1;
    outputPort.eDir = OMX_DirOutput;
    outputPort.nBufferAlignment = 2;
    outputPort.format.video.cMIMEType =
        const_cast<char *>(MEDIA_MIMETYPE_VIDEO_VPX);
    outputPort.format.video.eCompressionFormat = OMX_VIDEO_CodingVPX;
    outputPort.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    outputPort.format.video.pNativeWindow = NULL;
    outputPort.nBufferSize = 256 * 1024;  // arbitrary

    addPort(outputPort);
}


status_t SoftVPXEncoder::initEncoder() {
    vpx_codec_err_t codec_return;

    mCodecContext = new vpx_codec_ctx_t;
    mCodecConfiguration = new vpx_codec_enc_cfg_t;
    mCodecInterface = vpx_codec_vp8_cx();

    if (mCodecInterface == NULL) {
        return UNKNOWN_ERROR;
    }

    codec_return = vpx_codec_enc_config_default(mCodecInterface,
                                                mCodecConfiguration,
                                                0);  // Codec specific flags

    if (codec_return != VPX_CODEC_OK) {
        ALOGE("Error populating default configuration for vpx encoder.");
        return UNKNOWN_ERROR;
    }

    mCodecConfiguration->g_w = mWidth;
    mCodecConfiguration->g_h = mHeight;
    mCodecConfiguration->g_threads = GetCPUCoreCount();
    // OMX timebase unit is microsecond
    // g_timebase is in seconds (i.e. 1/1000000 seconds)
    mCodecConfiguration->g_timebase.num = 1;
    mCodecConfiguration->g_timebase.den = 1000000;
    // rc_target_bitrate is in kbps, mBitrate in bps
    mCodecConfiguration->rc_target_bitrate = mBitrate/1000;
    mCodecConfiguration->rc_end_usage = mBitrateControlMode;

    codec_return = vpx_codec_enc_init(mCodecContext,
                                      mCodecInterface,
                                      mCodecConfiguration,
                                      0);  // flags

    if (codec_return != VPX_CODEC_OK) {
        ALOGE("Error initializing vpx encoder");
        return UNKNOWN_ERROR;
    }

    if (mColorFormat == OMX_COLOR_FormatYUV420SemiPlanar) {
        if (mConversionBuffer == NULL) {
            mConversionBuffer = (uint8_t *)malloc(mWidth * mHeight * 3 / 2);
            if (mConversionBuffer == NULL) {
                ALOGE("Allocating conversion buffer failed.");
                return UNKNOWN_ERROR;
            }
        }
    }
    return OK;
}


status_t SoftVPXEncoder::releaseEncoder() {
    if (mCodecContext != NULL) {
        vpx_codec_destroy(mCodecContext);
        delete mCodecContext;
        mCodecContext = NULL;
    }

    if (mCodecConfiguration != NULL) {
        delete mCodecConfiguration;
        mCodecConfiguration = NULL;
    }

    if (mConversionBuffer != NULL) {
        delete mConversionBuffer;
        mConversionBuffer = NULL;
    }

    // this one is not allocated by us
    mCodecInterface = NULL;

    return OK;
}


OMX_ERRORTYPE SoftVPXEncoder::internalGetParameter(OMX_INDEXTYPE index,
                                                   OMX_PTR param) {
    switch (index) {
        case OMX_IndexParamVideoPortFormat: {
            OMX_VIDEO_PARAM_PORTFORMATTYPE *formatParams =
                (OMX_VIDEO_PARAM_PORTFORMATTYPE *)param;

            if (formatParams->nPortIndex == 0) {
                formatParams->eCompressionFormat = OMX_VIDEO_CodingUnused;
                formatParams->eColorFormat = mColorFormat;
                // Converting from microseconds
                // Also converting to Q16 format
                formatParams->xFramerate = (mFrameDuration*1000000) << 16;
                return OMX_ErrorNone;
            } else if (formatParams->nPortIndex == 1) {
                formatParams->eCompressionFormat = OMX_VIDEO_CodingVPX;
                formatParams->eColorFormat = OMX_COLOR_FormatUnused;
                formatParams->xFramerate = 0;
                return OMX_ErrorNone;
            } else {
                return OMX_ErrorBadPortIndex;
            }
        }

        case OMX_IndexParamVideoBitrate: {
            OMX_VIDEO_PARAM_BITRATETYPE *bitrate =
                (OMX_VIDEO_PARAM_BITRATETYPE *)param;

                if (bitrate->nPortIndex != 1) {
                    return OMX_ErrorUnsupportedIndex;
                }

                bitrate->nTargetBitrate = mBitrate;

                if (mBitrateControlMode == VPX_VBR) {
                    bitrate->eControlRate = OMX_Video_ControlRateVariable;
                } else if (mBitrateControlMode == VPX_CBR) {
                    bitrate->eControlRate = OMX_Video_ControlRateConstant;
                } else {
                    return OMX_ErrorUnsupportedSetting;
                }
                return OMX_ErrorNone;
        }

        default:
            return SimpleSoftOMXComponent::internalGetParameter(index, param);
    }
}


OMX_ERRORTYPE SoftVPXEncoder::internalSetParameter(OMX_INDEXTYPE index,
                                                   const OMX_PTR param) {
    switch (index) {
        case OMX_IndexParamStandardComponentRole:
            return internalSetRoleParams(
                (const OMX_PARAM_COMPONENTROLETYPE *)param);

        case OMX_IndexParamVideoBitrate:
            return internalSetBitrateParams(
                (const OMX_VIDEO_PARAM_BITRATETYPE *)param);

        case OMX_IndexParamPortDefinition:
            return internalSetPortParams(
                (const OMX_PARAM_PORTDEFINITIONTYPE *)param);

        case OMX_IndexParamVideoPortFormat:
            return internalSetFormatParams(
                (const OMX_VIDEO_PARAM_PORTFORMATTYPE *)param);

        default:
            return SimpleSoftOMXComponent::internalSetParameter(index, param);
    }
}


OMX_ERRORTYPE SoftVPXEncoder::internalSetFormatParams(
        const OMX_VIDEO_PARAM_PORTFORMATTYPE* format) {

    if (format->nPortIndex == 0) {
        if (format->eColorFormat == OMX_COLOR_FormatYUV420Planar ||
            format->eColorFormat == OMX_COLOR_FormatYUV420SemiPlanar ||
            format->eColorFormat == OMX_COLOR_FormatAndroidOpaque) {
            mColorFormat = format->eColorFormat;
            return OMX_ErrorNone;
        } else {
            ALOGE("Unsupported color format %i", format->eColorFormat);
            return OMX_ErrorUnsupportedSetting;
        }
    } else if (format->nPortIndex == 1) {
        if (format->eCompressionFormat == OMX_VIDEO_CodingVPX) {
            return OMX_ErrorNone;
        } else {
            return OMX_ErrorUnsupportedSetting;
        }
    } else {
        return OMX_ErrorBadPortIndex;
    }
}


OMX_ERRORTYPE SoftVPXEncoder::internalSetRoleParams(
        const OMX_PARAM_COMPONENTROLETYPE* role) {

    const char* roleText = (const char*)role->cRole;
    size_t roleTextMaxSize = OMX_MAX_STRINGNAME_SIZE - 1;

    if (strncmp(roleText, "video_encoder.vpx", roleTextMaxSize)) {
        ALOGE("Unsupported component role");
        return OMX_ErrorBadParameter;
    }

    return OMX_ErrorNone;
}


OMX_ERRORTYPE SoftVPXEncoder::internalSetPortParams(
        const OMX_PARAM_PORTDEFINITIONTYPE* port) {

    if (port->nPortIndex == 0) {
        mWidth = port->format.video.nFrameWidth;
        mHeight = port->format.video.nFrameHeight;

        // xFramerate comes in Q16 format, in frames per second unit
        int32_t framerate = port->format.video.xFramerate >> 16;
        // frame duration is in microseconds
        mFrameDuration = (1000000/framerate);

        if (port->format.video.eColorFormat == OMX_COLOR_FormatYUV420Planar ||
            port->format.video.eColorFormat == OMX_COLOR_FormatYUV420SemiPlanar ||
            port->format.video.eColorFormat == OMX_COLOR_FormatAndroidOpaque) {
                mColorFormat = port->format.video.eColorFormat;
        } else {
            return OMX_ErrorUnsupportedSetting;
        }

        return OMX_ErrorNone;
    } else if (port->nPortIndex == 1) {
        mBitrate = port->format.video.nBitrate;
        return OMX_ErrorNone;
    } else {
        return OMX_ErrorBadPortIndex;
    }
}


OMX_ERRORTYPE SoftVPXEncoder::internalSetBitrateParams(
        const OMX_VIDEO_PARAM_BITRATETYPE* bitrate) {

    if (bitrate->nPortIndex != 1) {
        return OMX_ErrorUnsupportedIndex;
    }

    mBitrate = bitrate->nTargetBitrate;

    if (bitrate->eControlRate == OMX_Video_ControlRateVariable) {
        mBitrateControlMode = VPX_VBR;
    } else if (bitrate->eControlRate == OMX_Video_ControlRateConstant) {
        mBitrateControlMode = VPX_CBR;
    } else {
        return OMX_ErrorUnsupportedSetting;
    }

    return OMX_ErrorNone;
}


void SoftVPXEncoder::onQueueFilled(OMX_U32 portIndex) {
    // Initialize encoder if not already
    if (mCodecContext == NULL) {
        if (OK != initEncoder()) {
            ALOGE("Failed to initialize encoder");
            return;
        }
    }

    vpx_codec_err_t codec_return;
    List<BufferInfo *> &inputBufferInfoQueue = getPortQueue(0);
    List<BufferInfo *> &outputBufferInfoQueue = getPortQueue(1);

    while (!inputBufferInfoQueue.empty() && !outputBufferInfoQueue.empty()) {
        BufferInfo *inputBufferInfo = *inputBufferInfoQueue.begin();
        OMX_BUFFERHEADERTYPE *inputBufferHeader = inputBufferInfo->mHeader;

        BufferInfo *outputBufferInfo = *outputBufferInfoQueue.begin();
        OMX_BUFFERHEADERTYPE *outputBufferHeader = outputBufferInfo->mHeader;

        if (inputBufferHeader->nFlags & OMX_BUFFERFLAG_EOS) {
            inputBufferInfoQueue.erase(inputBufferInfoQueue.begin());
            inputBufferInfo->mOwnedByUs = false;
            notifyEmptyBufferDone(inputBufferHeader);

            outputBufferHeader->nFilledLen = 0;
            outputBufferHeader->nFlags = OMX_BUFFERFLAG_EOS;

            outputBufferInfoQueue.erase(outputBufferInfoQueue.begin());
            outputBufferInfo->mOwnedByUs = false;
            notifyFillBufferDone(outputBufferHeader);
            return;
        }

        uint8_t* source = inputBufferHeader->pBuffer + inputBufferHeader->nOffset;

        if (mColorFormat == OMX_COLOR_FormatYUV420SemiPlanar) {
            ConvertYUV420SemiPlanarToYUV420Planar(source, mConversionBuffer, mWidth, mHeight);
            source = mConversionBuffer;
        }

        vpx_image_t raw_frame;
        vpx_img_wrap(&raw_frame, VPX_IMG_FMT_I420, mWidth, mHeight, mInputBufferAlignment, source);
        codec_return = vpx_codec_encode(mCodecContext,
                                        &raw_frame,
                                        inputBufferHeader->nTimeStamp,  // in timebase units
                                        mFrameDuration,  // frame duration in timebase units
                                        0,  // frame flags
                                        VPX_DL_REALTIME);  // encoding deadline
        if (codec_return != VPX_CODEC_OK) {
            ALOGE("vpx encoder failed to encode frame");
            notify(OMX_EventError, OMX_ErrorUndefined, 0, NULL);
            return;
        }

        vpx_codec_iter_t encoded_packet_iterator = NULL;
        const vpx_codec_cx_pkt_t* encoded_packet;

        while (encoded_packet = vpx_codec_get_cx_data(mCodecContext, &encoded_packet_iterator)) {
            if (encoded_packet->kind == VPX_CODEC_CX_FRAME_PKT) {
                outputBufferHeader->nTimeStamp = encoded_packet->data.frame.pts;
                outputBufferHeader->nFlags = 0;
                outputBufferHeader->nOffset = 0;
                outputBufferHeader->nFilledLen = encoded_packet->data.frame.sz;
                memcpy(outputBufferHeader->pBuffer,
                       encoded_packet->data.frame.buf,
                       encoded_packet->data.frame.sz);
                outputBufferInfo->mOwnedByUs = false;
                outputBufferInfoQueue.erase(outputBufferInfoQueue.begin());
                notifyFillBufferDone(outputBufferHeader);
            }
        }

        inputBufferInfo->mOwnedByUs = false;
        inputBufferInfoQueue.erase(inputBufferInfoQueue.begin());
        notifyEmptyBufferDone(inputBufferHeader);
    }
}
}  // namespace android


android::SoftOMXComponent *createSoftOMXComponent(
        const char *name, const OMX_CALLBACKTYPE *callbacks,
        OMX_PTR appData, OMX_COMPONENTTYPE **component) {
    return new android::SoftVPXEncoder(name, callbacks, appData, component);
}
