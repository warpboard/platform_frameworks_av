#
# This configure file is just for Linux projects against Android
#

VOPRJ :=
VONJ :=

# WARNING:
# Using v7 breaks generic build
ifeq ($(TARGET_ARCH),arm)
VOTT := v5
else
ifeq ($(TARGET_ARCH),mips)
VOTT := mips32
else
VOTT := pc
endif
endif

# Do we also need to check on ARCH_ARM_HAVE_ARMV7A? - probably not
ifeq ($(ARCH_ARM_HAVE_NEON),true)
VOTT := v7
endif

# Check if we have MIPS_DSP architecture
ifeq ($(ARCH_MIPS_HAS_DSP),true)
  ifeq ($(ARCH_MIPS_DSP_REV),1)
  VOTT := mips_dsp_r1
  else
  ifeq ($(ARCH_MIPS_DSP_REV),2)
  VOTT := mips_dsp_r2
  endif
endif
endif

VOTEST := 0

