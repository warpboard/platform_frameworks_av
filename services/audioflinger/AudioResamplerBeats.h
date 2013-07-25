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

#ifndef ANDROID_AUDIO_RESAMPLER_BEATS_H
#define ANDROID_AUDIO_RESAMPLER_BEATS_H

#include <stdint.h>
#include <sys/types.h>
#include <cutils/log.h>

#include "AudioResampler.h"

namespace android {
	// ----------------------------------------------------------------------------
    class AudioResamplerBeats : public AudioResampler {
    public:
        AudioResamplerBeats(int bitDepth, int inChannelCount, int32_t sampleRate, src_quality quality);
		
        virtual void resample(int32_t* out, size_t outFrameCount,
        AudioBufferProvider* provider);
		
        ~AudioResamplerBeats();
        struct resamp_state {
            int16_t* delayline_int16;
        };
		
    private:
        const unsigned int H_LOW_QUALITY_44_TO_48_INT16_LEN;
        const unsigned int H_MED_QUALITY_44_TO_48_INT16_LEN;
        const unsigned int H_HIGH_QUALITY_44_TO_48_INT16_LEN;
        const unsigned int H_VERY_HIGH_QUALITY_44_TO_48_INT16_LEN;
        const unsigned int H_LOW_QUALITY_48_TO_44_INT16_LEN;
        const unsigned int H_MED_QUALITY_48_TO_44_INT16_LEN;
        const unsigned int H_HIGH_QUALITY_48_TO_44_INT16_LEN;
        const unsigned int H_VERY_HIGH_QUALITY_48_TO_44_INT16_LEN;
		
        static const int16_t h_LOW_QUALITY_44_to_48_int16[160][16];
        static const int16_t h_MED_QUALITY_44_to_48_int16[160][16];
        static const int16_t h_HIGH_QUALITY_44_to_48_int16[160][24];
        static const int16_t h_VERY_HIGH_QUALITY_44_to_48_int16[160][32];
        static const int16_t h_LOW_QUALITY_48_to_44_int16[147][16];
        static const int16_t h_MED_QUALITY_48_to_44_int16[147][16];
        static const int16_t h_HIGH_QUALITY_48_to_44_int16[147][24];
        static const int16_t h_VERY_HIGH_QUALITY_48_to_44_int16[147][32];
		
        // Parameters that are common to all channels
        int mUpFactor;
        int mDnFactor;
        int mPpfiltLen;
        const int16_t** mppFilterTaps_int16;
        int mPpState;
		
        void init() {}
        void resampleMono16(int32_t* out, size_t outFrameCount,
        AudioBufferProvider* provider);
        void resampleStereo16(int32_t* out, size_t outFrameCount,
        AudioBufferProvider* provider);
        void initBeatsState(resamp_state* rs);
		
        // assuming two channels at most
        resamp_state left, right;
    };

    // ----------------------------------------------------------------------------
}; // namespace android

#endif /*ANDROID_AUDIO_RESAMPLER_BEATS_H*/
