/*
 * SPDX-FileCopyrightText: Copyright 2020-2024 Arm Limited and/or its affiliates <open-source-office@arm.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/****************************************************************************
 * Includes
 ****************************************************************************/

#include "target.hpp"

#ifdef ETHOSU
#include <ethosu_driver.h>
#endif

#include "mpu.hpp"
#include "uart_stdout.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

using namespace EthosU;

/****************************************************************************
 * Defines
 ****************************************************************************/

#define ETHOSU_BASE_ADDRESS 0x50004000
#define ETHOSU_IRQ          16
#define ETHOSU_IRQ_PRIORITY 5

/****************************************************************************
 * Variables
 ****************************************************************************/

#if defined(ETHOSU_FAST_MEMORY_SIZE) && ETHOSU_FAST_MEMORY_SIZE > 0
__attribute__((aligned(16), section(".bss.ethosu_scratch"))) uint8_t ethosu_scratch[ETHOSU_FAST_MEMORY_SIZE];
#else
#define ethosu_scratch          0
#define ETHOSU_FAST_MEMORY_SIZE 0
#endif

#ifdef ETHOSU
struct ethosu_driver ethosu0_driver;
#endif

/****************************************************************************
 * Cache maintenance
 ****************************************************************************/

#if defined(CPU_CACHE_ENABLE) && defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
extern "C" {
void ethosu_invalidate_dcache(uint32_t *p, size_t bytes) {
    SCB_InvalidateDCache_by_Addr(p, bytes);
}
}
#endif

/****************************************************************************
 * Init
 ****************************************************************************/

namespace {

extern "C" {
struct ExcContext {
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t xPsr;
};

void HardFault_Handler() {
    int irq;
    struct ExcContext *e;
    uint32_t sp;

    asm volatile("mrs %0, ipsr            \n" // Read IPSR (Exception number)
                 "sub %0, #16             \n" // Get it into IRQn_Type range
                 "tst lr, #4              \n" // Select the stack which was in use
                 "ite eq                  \n"
                 "mrseq %1, msp           \n"
                 "mrsne %1, psp           \n"
                 "mov %2, sp              \n"
                 : "=r"(irq), "=r"(e), "=r"(sp));

    printf("Hard fault. irq=%d, pc=0x%08" PRIx32 ", lr=0x%08" PRIx32 ", xpsr=0x%08" PRIx32 ", sp=0x%08" PRIx32 "\n",
           irq,
           e->pc,
           e->lr,
           e->xPsr,
           sp);
    printf(
        "%11s cfsr=0x%08" PRIx32 " bfar=0x%08" PRIx32 " mmfar=0x%08" PRIx32 "\n", "", SCB->CFSR, SCB->BFAR, SCB->MMFAR);
    exit(1);
}
}

#ifdef ETHOSU
void ethosuIrqHandler() {
    ethosu_irq_handler(&ethosu0_driver);
}
#endif

} // namespace

namespace EthosU {

void targetSetup() {
    // Initialize UART driver
    UartStdOutInit();

#ifdef ETHOSU
    // Initialize Ethos-U NPU driver
    if (ethosu_init(&ethosu0_driver,
                    reinterpret_cast<void *>(ETHOSU_BASE_ADDRESS),
                    ethosu_scratch,
                    ETHOSU_FAST_MEMORY_SIZE,
                    1,
                    1)) {
        printf("Failed to initialize NPU.\n");
        return;
    }

    // Assumes SCB->VTOR point to RW memory
    NVIC_SetVector(static_cast<IRQn_Type>(ETHOSU_IRQ), (uint32_t)&ethosuIrqHandler);
    NVIC_SetPriority(static_cast<IRQn_Type>(ETHOSU_IRQ), ETHOSU_IRQ_PRIORITY);
    NVIC_EnableIRQ(static_cast<IRQn_Type>(ETHOSU_IRQ));
#endif

    // MPU setup
    const std::vector<ARM_MPU_Region_t> mpuConfig = {
        {
            // ITCM (NS)
            ARM_MPU_RBAR(0x00000000,      // Base
                         ARM_MPU_SH_NON,  // Non-shareable
                         1,               // Read-Only
                         1,               // Non-Privileged
                         0),              // eXecute Never disabled
            ARM_MPU_RLAR(0x00007fff,      // Limit
                         Mpu::WTRA_index) // Attribute index - Write-Through, Read-allocate
        },
        {
            // ITCM (S)
            ARM_MPU_RBAR(0x10000000,      // Base
                         ARM_MPU_SH_NON,  // Non-shareable
                         1,               // Read-Only
                         1,               // Non-Privileged
                         0),              // eXecute Never disabled
            ARM_MPU_RLAR(0x10007fff,      // Limit
                         Mpu::WTRA_index) // Attribute index - Write-Through, Read-allocate
        },
        {
            // DTCM (NS)
            ARM_MPU_RBAR(0x20000000,        // Base
                         ARM_MPU_SH_NON,    // Non-shareable
                         0,                 // Read-Write
                         1,                 // Non-Privileged
                         1),                // eXecute Never enabled
            ARM_MPU_RLAR(0x20007fff,        // Limit
                         Mpu::WBWARA_index) // Attribute index - Write-Back, Write-Allocate, Read-allocate
        },
        {
            // DTCM (S)
            ARM_MPU_RBAR(0x30000000,        // Base
                         ARM_MPU_SH_NON,    // Non-shareable
                         0,                 // Read-Write
                         1,                 // Non-Privileged
                         1),                // eXecute Never enabled
            ARM_MPU_RLAR(0x30007fff,        // Limit
                         Mpu::WBWARA_index) // Attribute index - Write-Back, Write-Allocate, Read-allocate
        },
        {
            // FPGA DATA SRAM; BRAM (NS)
            ARM_MPU_RBAR(0x01000000,        // Base
                         ARM_MPU_SH_NON,    // Non-shareable
                         0,                 // Read-Write
                         1,                 // Non-Privileged
                         0),                // eXecute Never disabled
            ARM_MPU_RLAR(0x011fffff,        // Limit
                         Mpu::WBWARA_index) // Attribute index - Write-Back, Write-Allocate, Read-allocate
        },
        {
            // FPGA DATA SRAM; BRAM (S)
            ARM_MPU_RBAR(0x11000000,        // Base
                         ARM_MPU_SH_NON,    // Non-shareable
                         0,                 // Read-Write
                         1,                 // Non-Privileged
                         0),                // eXecute Never disabled
            ARM_MPU_RLAR(0x111fffff,        // Limit
                         Mpu::WBWARA_index) // Attribute index - Write-Back, Write-Allocate, Read-allocate
        },
        {
            // SSE-300 internal SRAM (NS)
            ARM_MPU_RBAR(0x21000000,        // Base
                         ARM_MPU_SH_NON,    // Non-shareable
                         0,                 // Read-Write
                         1,                 // Non-Privileged
                         0),                // eXecute Never disabled
            ARM_MPU_RLAR(0x213fffff,        // Limit
                         Mpu::WTWARA_index) // Attribute index - Write-Through, Write-Allocate, Read-allocate
        },
        {
            // SSE-300 internal SRAM (S)
            ARM_MPU_RBAR(0x31000000,        // Base
                         ARM_MPU_SH_NON,    // Non-shareable
                         0,                 // Read-Write
                         1,                 // Non-Privileged
                         0),                // eXecute Never disabled
            ARM_MPU_RLAR(0x313fffff,        // Limit
                         Mpu::WTWARA_index) // Attribute index - Write-Through, Write-Allocate, Read-allocate
        },
        {
            // DDR (NS)
            ARM_MPU_RBAR(0x60000000,        // Base
                         ARM_MPU_SH_NON,    // Non-shareable
                         0,                 // Read-Write
                         1,                 // Non-Privileged
                         1),                // eXecute Never enabled
            ARM_MPU_RLAR(0x6fffffff,        // Limit
                         Mpu::WBWARA_index) // Attribute index - Write-Back, Write-Allocate, Read-allocate
        },
        {
            // DDR (S)
            ARM_MPU_RBAR(0x70000000,        // Base
                         ARM_MPU_SH_NON,    // Non-shareable
                         0,                 // Read-Write
                         1,                 // Non-Privileged
                         1),                // eXecute Never enabled
            ARM_MPU_RLAR(0x7fffffff,        // Limit
                         Mpu::WBWARA_index) // Attribute index - Write-Back, Write-Allocate, Read-allocate
        }};

    // Setup MPU configuration
    Mpu::loadAndEnableConfig(&mpuConfig[0], mpuConfig.size());

#if defined(CPU_CACHE_ENABLE) && defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
    SCB_EnableICache();
    SCB_EnableDCache();
#endif
}

} // namespace EthosU
