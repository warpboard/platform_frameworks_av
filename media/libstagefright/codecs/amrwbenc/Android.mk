LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
include frameworks/av/media/libstagefright/codecs/common/Config.mk



LOCAL_SRC_FILES := \
	src/autocorr.c \
	src/az_isp.c \
	src/bits.c \
	src/c2t64fx.c \
	src/c4t64fx.c \
	src/convolve.c \
	src/cor_h_x.c \
	src/decim54.c \
	src/deemph.c \
	src/dtx.c \
	src/g_pitch.c \
	src/gpclip.c \
	src/homing.c \
	src/hp400.c \
	src/hp50.c \
	src/hp6k.c \
	src/hp_wsp.c \
	src/int_lpc.c \
	src/isp_az.c \
	src/isp_isf.c \
	src/lag_wind.c \
	src/levinson.c \
	src/log2.c \
	src/lp_dec2.c \
	src/math_op.c \
	src/oper_32b.c \
	src/p_med_ol.c \
	src/pit_shrp.c \
	src/pitch_f4.c \
	src/pred_lt4.c \
	src/preemph.c \
	src/q_gain2.c \
	src/q_pulse.c \
	src/qisf_ns.c \
	src/qpisf_2s.c \
	src/random.c \
	src/residu.c \
	src/scale.c \
	src/stream.c \
	src/syn_filt.c \
	src/updt_tar.c \
	src/util.c \
	src/voAMRWBEnc.c \
	src/voicefac.c \
	src/wb_vad.c \
	src/weight_a.c \
	src/mem_align.c


ifeq ($(VOTT), v5)
LOCAL_SRC_FILES += \
	src/asm/ARMV5E/convolve_opt.s \
	src/asm/ARMV5E/cor_h_vec_opt.s \
	src/asm/ARMV5E/Deemph_32_opt.s \
	src/asm/ARMV5E/Dot_p_opt.s \
	src/asm/ARMV5E/Filt_6k_7k_opt.s \
	src/asm/ARMV5E/Norm_Corr_opt.s \
	src/asm/ARMV5E/pred_lt4_1_opt.s \
	src/asm/ARMV5E/residu_asm_opt.s \
	src/asm/ARMV5E/scale_sig_opt.s \
	src/asm/ARMV5E/Syn_filt_32_opt.s \
	src/asm/ARMV5E/syn_filt_opt.s

endif

ifeq ($(VOTT), v7)
LOCAL_SRC_FILES += \
	src/asm/ARMV7/convolve_neon.s \
	src/asm/ARMV7/cor_h_vec_neon.s \
	src/asm/ARMV7/Deemph_32_neon.s \
	src/asm/ARMV7/Dot_p_neon.s \
	src/asm/ARMV7/Filt_6k_7k_neon.s \
	src/asm/ARMV7/Norm_Corr_neon.s \
	src/asm/ARMV7/pred_lt4_1_neon.s \
	src/asm/ARMV7/residu_asm_neon.s \
	src/asm/ARMV7/scale_sig_neon.s \
	src/asm/ARMV7/Syn_filt_32_neon.s \
	src/asm/ARMV7/syn_filt_neon.s

endif

ifeq ($(VOTT), mips32)
LOCAL_SRC_FILES += \
	src/asm/mips/autocorr_mips.c \
	src/asm/mips/az_isp_mips.c \
	src/asm/mips/c4t64fx_mips.c \
	src/asm/mips/convolve_mips.c \
	src/asm/mips/cor_h_x_mips.c \
	src/asm/mips/decim54_mips.c \
	src/asm/mips/p_med_ol_mips.c \
	src/asm/mips/pitch_f4_mips.c \
	src/asm/mips/pred_lt4_mips.c \
	src/asm/mips/qpisf_2s_mips.c \
	src/asm/mips/scale_mips.c \
	src/asm/mips/syn_filt_mips.c
endif

ifeq ($(VOTT), mips_dsp_r1)
LOCAL_SRC_FILES += \
	src/asm/mips/autocorr_mips.c \
	src/asm/mips/az_isp_mips.c \
	src/asm/mips/c2t64fx_mips.c \
	src/asm/mips/c4t64fx_mips.c \
	src/asm/mips/convolve_mips.c \
	src/asm/mips/cor_h_x_mips.c \
	src/asm/mips/decim54_mips.c \
	src/asm/mips/hp6k_mips.c \
	src/asm/mips/oper_32b_mips.c \
	src/asm/mips/p_med_ol_mips.c \
	src/asm/mips/pitch_f4_mips.c \
	src/asm/mips/pred_lt4_mips.c \
	src/asm/mips/qpisf_2s_mips.c \
	src/asm/mips/residu_mips.c \
	src/asm/mips/scale_mips.c \
	src/asm/mips/syn_filt_mips.c
endif

ifeq ($(VOTT), mips_dsp_r2)
LOCAL_SRC_FILES += \
	src/asm/mips/autocorr_mips.c \
	src/asm/mips/az_isp_mips.c \
	src/asm/mips/c2t64fx_mips.c \
	src/asm/mips/c4t64fx_mips.c \
	src/asm/mips/convolve_mips.c \
	src/asm/mips/cor_h_x_mips.c \
	src/asm/mips/decim54_mips.c \
	src/asm/mips/hp6k_mips.c \
	src/asm/mips/oper_32b_mips.c \
	src/asm/mips/p_med_ol_mips.c \
	src/asm/mips/pitch_f4_mips.c \
	src/asm/mips/pred_lt4_mips.c \
	src/asm/mips/qpisf_2s_mips.c \
	src/asm/mips/residu_mips.c \
	src/asm/mips/scale_mips.c \
	src/asm/mips/syn_filt_mips.c
endif

LOCAL_MODULE := libstagefright_amrwbenc

LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES :=

LOCAL_SHARED_LIBRARIES :=

LOCAL_C_INCLUDES := \
	frameworks/av/include \
	frameworks/av/media/libstagefright/include \
	frameworks/av/media/libstagefright/codecs/common/include \
	$(LOCAL_PATH)/src \
	$(LOCAL_PATH)/inc

ifeq ($(VOTT), v5)
LOCAL_CFLAGS += -DARM -DASM_OPT
LOCAL_C_INCLUDES += $(LOCAL_PATH)/src/asm/ARMV5E
endif

ifeq ($(VOTT), v7)
LOCAL_CFLAGS += -DARM -DARMV7 -DASM_OPT
LOCAL_C_INCLUDES += $(LOCAL_PATH)/src/asm/ARMV5E
LOCAL_C_INCLUDES += $(LOCAL_PATH)/src/asm/ARMV7
endif

ifeq ($(VOTT), mips32)
LOCAL_CFLAGS += -DMIPS32_LE
LOCAL_C_INCLUDES += $(LOCAL_PATH)/src/asm/mips
endif

ifeq ($(VOTT), mips_dsp_r1)
LOCAL_CFLAGS += -DMIPS32_LE -DMIPS32_R2_LE -DMIPS_DSP_R1_LE
endif

ifeq ($(VOTT), mips_dsp_r2)
LOCAL_CFLAGS += -DMIPS32_LE -DMIPS32_R2_LE -DMIPS_DSP_R1_LE -DMIPS_DSP_R2_LE
endif

include $(BUILD_STATIC_LIBRARY)

################################################################################

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
        SoftAMRWBEncoder.cpp

LOCAL_C_INCLUDES := \
	frameworks/av/media/libstagefright/include \
	frameworks/av/media/libstagefright/codecs/common/include \
	frameworks/native/include/media/openmax

LOCAL_STATIC_LIBRARIES := \
        libstagefright_amrwbenc

LOCAL_SHARED_LIBRARIES := \
        libstagefright_omx libstagefright_foundation libutils \
        libstagefright_enc_common

LOCAL_MODULE := libstagefright_soft_amrwbenc
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
