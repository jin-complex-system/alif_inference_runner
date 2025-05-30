/*
 * SPDX-FileCopyrightText: Copyright 2009-2024 Arm Limited and/or its affiliates <open-source-office@arm.com>
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

 /*
 * This is a simplified picture of the Corstone-300 memory system.
 * Please refer to the Corstone SSE-300 Technical Reference Manual for
 * further information.
 *
 * https://developer.arm.com/ip-products/subsystem/corstone/corstone-300
 *
 * +---------------+   +---------------+  +------+
 * | Ethos-U55     |   | Cortex-M55    +--+ ITCM |
 * |               |   |               |  +------+
 * |               |   |               |
 * |               |   |               |  +------+
 * |  M1      M0   |   |               +--+ DTCM |
 * +---+-------+---+   +-------+-------+  +------+
 *     |       |               |
 *     |   +---+---------------+-----+
 *     |   | AMBA AXI NIC-400-Lite   |
 *     |   +---+-----------------+---+
 *     |       |                 |
 * +---+-------+------------+ +--+-------+
 * | AMBA AXI NIC-400       | | SSE-300  |
 * +---+--------+--------+--+ | SRAM     |
 *     |        |        |    +----------+
 * +---+---+ +--+---+ +--+--+
 * | Flash | | BRAM | | DDR |
 * +-------+ +------+ +-----+
 *
 * +-----------------------+-------------+-------------+----+--------------------------------------+
 * |  Memory region name   | Base addr   |    Size     |IDAU|  MCC load address + remarks          |
 * +-----------------------+-------------+-------------+----+--------------------------------------+
 * | ITCM                  | 0x0000_0000 | 0x0008_0000 | NS | 0x0000_0000; 512 kiB                 |
 * | ITCM                  | 0x1000_0000 | 0x0008_0000 | S  | Secure alias for NS ITCM             |
 * | FPGA Data SRAM; BRAM  | 0x0100_0000 | 0x0010_0000 | NS | 0x0100_0000; 1 MiB                   |
 * | FPGA data SRAM; BRAM  | 0x1100_0000 | 0x0010_0000 | S  | Secure alias for NS BRAM             |
 * | DTCM                  | 0x2000_0000 | 0x0008_0000 | NS | 512 kiB; 4 banks of 128k each        |
 * | DTCM                  | 0x3000_0000 | 0x0008_0000 | S  | Secure alias for NS DTCM             |
 * | SSE-300 internal SRAM | 0x2100_0000 | 0x0020_0000 | NS | 1 bank of 2 MiB; 3cc latency)        |
 * | SSE-300 internal SRAM | 0x3100_0000 | 0x0020_0000 | S  | Secure alias for NS internal SRAM    |
 * | QSPI external flash   | 0x2800_0000 | 0x0080_0000 | NS | 8MB                                  |
 * | QSPI external flash   | 0x3800_0000 | 0x0080_0000 | S  | 8MB                                  |
 * | DDR                   | 0x6000_0000 | 0x1000_0000 | NS | 0x0800_0000; 256 MiB bank            |
 * | DDR                   | 0x7000_0000 | 0x1000_0000 | S  | 0x0C00_0000; 256 MiB bank            |
 * +-----------------------+-------------+-------------+----+--------------------------------------+
 *
 *  Note: Ethos-U55 can access BRAM, internal SRAM and the DDR sections => activation buffers and
 *        the model should only be placed in those regions.
 *
 *  Note: Alias regions means that secure and non-secure addresses are mapped to the same physical
 *        memory banks.
 */

#ifndef ETHOSU_MODEL
  /* default value - '1', for DRAM */
  #define ETHOSU_MODEL  1
#endif

#ifndef ETHOSU_ARENA
  /* default value - '1', for DRAM */
  #define ETHOSU_ARENA  1
#endif

__STACK_SIZE = 0x00008000;
__HEAP_SIZE  = 0x00008000;

MEMORY
{
  ITCM  (rx)  : ORIGIN = 0x10000000, LENGTH = 0x00080000
  BRAM  (rw)  : ORIGIN = 0x11000000, LENGTH = 0x00100000
  DTCM  (rw)  : ORIGIN = 0x30000000, LENGTH = 0x00080000
  SRAM  (rw)  : ORIGIN = 0x31000000, LENGTH = 0x00200000
  QSPI  (rw)  : ORIGIN = 0x38000000, LENGTH = 0x00800000
  DDR   (rwx) : ORIGIN = 0x70000000, LENGTH = 0x60000000
}

PHDRS
{
    rom_exec PT_LOAD;
    rom_dram PT_LOAD;
    null     PT_NULL;
}

/* Linker script to place sections and symbol values. Should be used together
 * with other linker script that defines memory regions ITCM and RAM.
 * It references following symbols, which must be defined in code:
 *   Reset_Handler : Entry of reset handler
 *
 * It defines following symbols, which code can use without definition:
 *   __exidx_start
 *   __exidx_end
 *   __copy_table_start__
 *   __copy_table_end__
 *   __zero_table_start__
 *   __zero_table_end__
 *   __etext
 *   __data_start__
 *   __preinit_array_start
 *   __preinit_array_end
 *   __init_array_start
 *   __init_array_end
 *   __fini_array_start
 *   __fini_array_end
 *   __data_end__
 *   __bss_start__
 *   __bss_end__
 *   __end__
 *   end
 *   __HeapLimit
 *   __StackLimit
 *   __StackTop
 *   __stack
 */

ENTRY(Reset_Handler)

SECTIONS
{
  .text :
  {
    _vectors = .;
    KEEP(*(.vectors))
    *(.text*)

    KEEP(*(.init))
    KEEP(*(.fini))

    /* .ctors */
    *crtbegin.o(.ctors)
    *crtbegin?.o(.ctors)
    *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
    *(SORT(.ctors.*))
    *(.ctors)

    /* .dtors */
    *crtbegin.o(.dtors)
    *crtbegin?.o(.dtors)
    *(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
    *(SORT(.dtors.*))
    *(.dtors)

    KEEP(*(.eh_frame*))
  } > ITCM :rom_exec

  /*
   * SG veneers:
   * All SG veneers are placed in the special output section .gnu.sgstubs. Its start address
   * must be set, either with the command line option '--section-start' or in a linker script,
   * to indicate where to place these veneers in memory.
   */
/*
  .gnu.sgstubs :
  {
    . = ALIGN(32);
  } > ITCM :rom_exec
*/
  .ARM.extab :
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } > ITCM :rom_exec

  .ARM.exidx :
  {
  __exidx_start = .;
    *(.ARM.exidx* .gnu.linkonce.armexidx.*)
  __exidx_end = .;
  } > ITCM :rom_exec

  .copy.table :
  {
    . = ALIGN(4);
    __copy_table_start__ = .;
    LONG (__etext)
    LONG (__data_start__)
    LONG ((__data_end__ - __data_start__) / 4)

    LONG (__eddr_data)
    LONG (__sram_data_start__)
    LONG ((__sram_data_end__ - __sram_data_start__) / 4)

    LONG (__eddr_data + (__sram_data_end__ - __sram_data_start__))
    LONG (__rodata_start__)
    LONG ((__rodata_end__ - __rodata_start__) / 4)

    __copy_table_end__ = .;
  } > ITCM :rom_exec

  .zero.table :
  {
    . = ALIGN(4);
    __zero_table_start__ = .;
    LONG (__bss_start__)
    LONG ((__bss_end__ - __bss_start__) / 4)
    __zero_table_end__ = .;

  /**
   * Location counter can end up 2byte aligned with narrow Thumb code but
   * __etext is assumed by startup code to be the LMA of a section in DTCM
   * which must be 4byte aligned
   */
  __etext = ALIGN (4);

  } > ITCM :rom_exec

  .data : AT(__etext)
  {
    __data_start__ = .;
    *(vtable)
    *(.data)
    *(.data.*)

    . = ALIGN(4);
    /* preinit data */
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP(*(.preinit_array))
    PROVIDE_HIDDEN (__preinit_array_end = .);

    . = ALIGN(4);
    /* init data */
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array))
    PROVIDE_HIDDEN (__init_array_end = .);

    . = ALIGN(4);
    /* finit data */
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array))
    PROVIDE_HIDDEN (__fini_array_end = .);

    KEEP(*(.jcr*))
    . = ALIGN(4);
    /* All data end */
    __data_end__ = .;
  } > DTCM :rom_exec

  .sram.bss :
  {
    . = ALIGN(16);
#if (ETHOSU_MODEL == 0)
  * (network_model_sec)
#endif

#if (ETHOSU_ARENA == 0)
    . = ALIGN(32);
    *(.bss.tensor_arena)
#endif
    . = ALIGN(16);
    *(.bss.ethosu_scratch);
    *.(output_data_sec)
  } > SRAM :null

  .ddr :
  {
#if (ETHOSU_ARENA == 1)
    . = ALIGN(32);
    *(.bss.tensor_arena)
#endif

    . = ALIGN(4);
    *(input_data_sec)
    . = ALIGN(16);
#if (ETHOSU_MODEL == 1)
    *(network_model_sec)
#endif
    * (expected_output_data_sec)
    * (sec_command_stream, sec_weight_data, sec_input_data)

    * (ethosu_core_in_queue)
    * (ethosu_core_out_queue)
    . = ALIGN(4);
  } > DDR :rom_dram

  __eddr_data = ALIGN(4);
  .sram.data :
  {
    __sram_data_start__ = .;
    *(.sram.data)
    . = ALIGN(4);
    __sram_data_end__ = .;
  } > BRAM AT >DDR :rom_dram

  .rodata :
  {
    __rodata_start__ = .;
    *(.rodata)
    *(.rodata.*)
    . = ALIGN(4);
    __rodata_end__ = .;
  } > DTCM AT >DDR :rom_dram

  .bss :
  {
    . = ALIGN(4);
    __bss_start__ = .;
    *(.bss)
    *(.bss.*)
    *(COMMON)
    . = ALIGN(4);
    __bss_end__ = .;
  } > DTCM :null

  .heap (COPY) :
  {
    . = ALIGN(8);
    __end__ = .;
    PROVIDE(end = .);
    . = . + __HEAP_SIZE;
    . = ALIGN(8);
    __HeapLimit = .;
  } > DTCM :null

  .stack (ORIGIN(DTCM) + LENGTH(DTCM) - __STACK_SIZE) (COPY) :
  {
    . = ALIGN(8);
    __StackLimit = .;
    . = . + __STACK_SIZE;
    . = ALIGN(8);
    __StackTop = .;
  } > DTCM :null
  PROVIDE(__stack = __StackTop);

  __RAM_segment_used_end__ = .;

  /* Check if data + heap + stack exceeds DTCM limit */
  ASSERT(__StackLimit >= __HeapLimit, "region DTCM overflowed with stack")
}
