# Table of Contents

- [CMSIS-DSP Statistics Internal Compilation Error](#cmsis-dsp-statistics-internal-compilation-error)
- [C Compiltation Error](#c-compiltation-error)


# CMSIS-DSP Statistics Internal Compilation Error
Known issue when building half-float support for [CMSIS-DSP](https://github.com/ARM-software/CMSIS-DSP/issues/242)

## Workaround

Three workarounds (the last one is the one that only works):
1. Disabling half-float support in [CMakeLists.txt](alif_ml-embedded-evaluation-kit/dependencies/cmsis-dsp/Source/CMakeLists.txt) manually
```bash
# option(DISABLEFLOAT16 "Disable building float16 kernels" OFF)
```

2. Remove half float support in [Config.cmake](alif_ml-embedded-evaluation-kit/dependencies/cmsis-dsp/Source/StatisticsFunctions/Config.cmake)
```bash
# if ((NOT ARMAC5) AND (NOT DISABLEFLOAT16))
# target_sources(CMSISDSP PRIVATE StatisticsFunctions/arm_max_f16.c)
# target_sources(CMSISDSP PRIVATE StatisticsFunctions/arm_min_f16.c)
# target_sources(CMSISDSP PRIVATE StatisticsFunctions/arm_mean_f16.c)
# target_sources(CMSISDSP PRIVATE StatisticsFunctions/arm_power_f16.c)
# target_sources(CMSISDSP PRIVATE StatisticsFunctions/arm_rms_f16.c)
# target_sources(CMSISDSP PRIVATE StatisticsFunctions/arm_std_f16.c)
# target_sources(CMSISDSP PRIVATE StatisticsFunctions/arm_var_f16.c)
# target_sources(CMSISDSP PRIVATE StatisticsFunctions/arm_entropy_f16.c)
# target_sources(CMSISDSP PRIVATE StatisticsFunctions/arm_kullback_leibler_f16.c)
# target_sources(CMSISDSP PRIVATE StatisticsFunctions/arm_logsumexp_dot_prod_f16.c)
# target_sources(CMSISDSP PRIVATE StatisticsFunctions/arm_logsumexp_f16.c)
# target_sources(CMSISDSP PRIVATE StatisticsFunctions/arm_max_no_idx_f16.c)
# target_sources(CMSISDSP PRIVATE StatisticsFunctions/arm_min_no_idx_f16.c)

# target_sources(CMSISDSP PRIVATE StatisticsFunctions/arm_absmax_f16.c)
# target_sources(CMSISDSP PRIVATE StatisticsFunctions/arm_absmin_f16.c)
# target_sources(CMSISDSP PRIVATE StatisticsFunctions/arm_absmax_no_idx_f16.c)
# target_sources(CMSISDSP PRIVATE StatisticsFunctions/arm_absmin_no_idx_f16.c)
# target_sources(CMSISDSP PRIVATE StatisticsFunctions/arm_accumulate_f16.c)
# endif()
```

3. Force return of bad results in problematic [`arm_min_no_idx_f16.c`](alif_ml-embedded-evaluation-kit/dependencies\cmsis-dsp\Source\StatisticsFunctions\arm_min_no_idx_f16.c):
- Line 61:
```
ARM_DSP_ATTRIBUTE void arm_min_no_idx_f16(
    const float16_t *pSrc,
    uint32_t   blockSize,
    float16_t *pResult)
{
   return; // Force the compiler to not build the bottom code
   f16x8_t     vecSrc;
   f16x8_t     curExtremValVec = vdupq_n_f16(F16_MAX);
   float16_t   minValue = F16_MAX;
   float16_t   newVal;
   uint32_t    blkCnt;
```

- Line 117:
```
ARM_DSP_ATTRIBUTE void arm_min_no_idx_f16(
    const float16_t *pSrc,
    uint32_t   blockSize,
    float16_t *pResult)
{
   return; // Force the compiler to not build the bottom code
   float16_t   minValue = F16_MAX;
   float16_t   newVal;
```

# C Compiltation Error

## Workaround
- In [`dependencies/boardlib/devkit_gen2`](alif_ml-embedded-evaluation-kit/dependencies/boardlib/devkit_gen2), change the following:
```C
void BOARD_BUTTON1_Init(BOARD_Callback_t user_cb)
{
	ARM_DRIVER_GPIO *BOARD_BUTTON1_GPIOdrv = &ARM_Driver_GPIO_(BOARD_BUTTON1_GPIO_PORT);
	// BOARD_BUTTON1_GPIOdrv->Initialize(BOARD_BUTTON1_PIN_NO, user_cb);
    BOARD_BUTTON1_GPIOdrv->Initialize(BOARD_BUTTON1_PIN_NO, NULL);
}

void BOARD_BUTTON2_Init(BOARD_Callback_t user_cb)
{
	ARM_DRIVER_GPIO *BOARD_BUTTON2_GPIOdrv = &ARM_Driver_GPIO_(BOARD_BUTTON2_GPIO_PORT);
	// BOARD_BUTTON2_GPIOdrv->Initialize(BOARD_BUTTON2_PIN_NO, user_cb);
    BOARD_BUTTON2_GPIOdrv->Initialize(BOARD_BUTTON2_PIN_NO, NULL);
}
```
