/*
 * SPDX-FileCopyrightText: Copyright 2021-2022 Arm Limited and/or its affiliates <open-source-office@arm.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "timer_simple_platform.h"

#include "log_macros.h"     /* Logging macros. */
#include "RTE_Components.h" /* CPU definitions and functions. */

#include <inttypes.h>

static uint64_t cpu_cycle_count = 0;    /* 64-bit cpu cycle counter */
extern uint32_t SystemCoreClock;        /* Expected to come from the cmsis-device lib */

/**
 * @brief   Gets the system tick triggered cycle counter for the CPU.
 * @return  64-bit counter value.
 **/
static uint64_t Get_SysTick_Cycle_Count(void);

/**
 * SysTick initialisation
 */
static int Init_SysTick(void);

/**
 * @brief Adds one PMU counter to the counters' array
 * @param value Value of the counter
 * @param name  Name for the given counter
 * @param unit  Unit for the "value"
 * @param counters Pointer to the counter struct - the one to be populated.
 * @return true if successfully added, false otherwise
 */
static bool add_pmu_counter(
        uint64_t value,
        const char* name,
        const char* unit,
        pmu_counters* counters);

void platform_reset_counters(void)
{
    if (0 != Init_SysTick()) {
        printf_err("Failed to initialise system tick config\n");
        return;
    }

#if defined(ARM_NPU)
    ethosu_pmu_init();
#endif /* defined (ARM_NPU) */

    debug("system tick config ready\n");
}

void platform_get_counters(pmu_counters* counters)
{
    counters->num_counters = 0;
    counters->initialised = true;
    uint32_t i = 0;

#if defined (ARM_NPU)
    ethosu_pmu_counters npu_counters = ethosu_get_pmu_counters();
    for (i = 0; i < ETHOSU_USED_PMU_NCOUNTERS; ++i) {
        add_pmu_counter(
                npu_counters.npu_evt_counters[i].counter_value,
                npu_counters.npu_evt_counters[i].name,
                npu_counters.npu_evt_counters[i].unit,
                counters);
    }
    for (i = 0; i < ETHOSU_DERIVED_NCOUNTERS; ++i) {
        add_pmu_counter(
                npu_counters.npu_derived_counters[i].counter_value,
                npu_counters.npu_derived_counters[i].name,
                npu_counters.npu_derived_counters[i].unit,
                counters);
    }
    add_pmu_counter(
            npu_counters.npu_total_ccnt,
            "NPU TOTAL",
            "cycles",
            counters);
#else  /* defined (ARM_NPU) */
    UNUSED(i);
#endif /* defined (ARM_NPU) */

#if defined(CPU_PROFILE_ENABLED)
    add_pmu_counter(
            Get_SysTick_Cycle_Count(),
            "CPU TOTAL",
            "cycles",
            counters);
#endif /* defined(CPU_PROFILE_ENABLED) */

#if !defined(CPU_PROFILE_ENABLED)
    UNUSED(Get_SysTick_Cycle_Count);
#if !defined(ARM_NPU)
    UNUSED(add_pmu_counter);
#endif /* !defined(ARM_NPU) */
#endif /* !defined(CPU_PROFILE_ENABLED) */
}

void SysTick_Handler(void)
{
    /* Increment the cycle counter based on load value. */
    cpu_cycle_count += SysTick->LOAD + 1;
}

/**
 * Gets the current SysTick derived counter value
 */
static uint64_t Get_SysTick_Cycle_Count(void)
{
    uint32_t systick_val;

    NVIC_DisableIRQ(SysTick_IRQn);
    systick_val = SysTick->VAL & SysTick_VAL_CURRENT_Msk;
    NVIC_EnableIRQ(SysTick_IRQn);

    return cpu_cycle_count + (SysTick->LOAD - systick_val);
}

/**
 * SysTick initialisation
 */
static int Init_SysTick(void)
{
    const uint32_t ticks_10ms = SystemCoreClock/100 + 1;
    int err = 0;

    /* Reset CPU cycle count value. */
    cpu_cycle_count = 0;

    /* Changing configuration for sys tick => guard from being
     * interrupted. */
    NVIC_DisableIRQ(SysTick_IRQn);

    /* SysTick init - this will enable interrupt too. */
    err = SysTick_Config(ticks_10ms);

    /* Enable interrupt again. */
    NVIC_EnableIRQ(SysTick_IRQn);

    /* Wait for SysTick to kick off */
    while (!err && !SysTick->VAL) {
        __NOP();
    }

    return err;
}

static bool add_pmu_counter(uint64_t value,
                            const char* name,
                            const char* unit,
                            pmu_counters* counters)
{
    const uint32_t idx = counters->num_counters;
    if (idx < NUM_PMU_COUNTERS) {
        counters->counters[idx].value = value;
        counters->counters[idx].name = name;
        counters->counters[idx].unit = unit;
        ++counters->num_counters;
        return true;
    }
    printf_err("Failed to add PMU counter!\n");
    return false;
}
