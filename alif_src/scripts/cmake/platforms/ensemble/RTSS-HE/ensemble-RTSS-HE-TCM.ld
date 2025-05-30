/* This file was ported to work on Alif Semiconductor Ensemble family of devices. */

/* Copyright (C) 2022 Alif Semiconductor - All Rights Reserved.
 * Use, distribution and modification of this code is permitted under the
 * terms stated in the Alif Semiconductor Software License Agreement
 *
 * You should have received a copy of the Alif Semiconductor Software
 * License Agreement with this file. If not, please write to:
 * contact@alifsemi.com, or visit: https://alifsemi.com/license
 *
 */

/*
 * Copyright (c) 2021 Arm Limited. All rights reserved.
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

__STACK_SIZE = 0x00001800;
__HEAP_SIZE  = 0x00008000;
__ROM_BASE = 0x80480000;
__ROM_SIZE = 0x000FF000;

MEMORY
{
  ITCM  (rwx) : ORIGIN = 0x01000000, LENGTH = 0x00040000
  DTCM  (rwx) : ORIGIN = 0x20000000, LENGTH = 0x00040000
  MRAM  (rx)  : ORIGIN = __ROM_BASE, LENGTH = __ROM_SIZE
}

ENTRY(Reset_Handler)

SECTIONS
{
  .startup.at_mram : ALIGN(16)
  {
    /* MRAM has start-up code, and most constant data */
    /* Only more-specific stuff here. General wildcards in
     * a later-declared section, giving a chance for other
     * regions to get in before-hand
     */
    KEEP(*(.vectors))

    *startup_M55*.c.obj(.text .rodata*)
    *system_M55*.c.obj(.text .rodata*)
    *mpu_M55*.c.obj(.text .rodata*)
    *Main.cc.obj(.text .rodata*)
    *platform_drivers.c.obj(.text .rodata*)
    *tcm_partition.c.obj(.text* .rodata*)

    *liblvgl.a:*(.text .rodata*)
    *libArm-2D.a:*(.text* .rodata*)
    *libcmsis_device.a:*(.text* .rodata*)
    *libensemble_services.a:*(.text* .rodata*)
    *liblcd_*.a:*(.text* .rodata*)
    *libimage_*.a:*(.text* .rodata*)
    *libcommon_api.a:*(.text* .rodata*)
    *libalif_ui_api.a:*(.text* .rodata*)
    *libg.a:*printf*(.text* .rodata*)
    *libstdc++.a:cp-demangle.o(.text* .rodata*)
    *libtensorflow-microlite.a:arm_*_s4.o(.text* .rodata*)
    *libtensorflow-microlite.a:arm_*_s16.o(.text* .rodata*)

    *(startup_ro_data)
  } > MRAM

  .copy.table.at_mram : ALIGN(4)
  {
    __copy_table_start__ = .;
    LONG ( LOADADDR(.text.itcm.at_mram) )
    LONG ( ADDR(.text.itcm.at_mram) )
    LONG ( SIZEOF(.text.itcm.at_mram)/4 )
    LONG ( LOADADDR(.data.dtcm.at_mram) )
    LONG ( ADDR(.data.dtcm.at_mram) )
    LONG ( SIZEOF(.data.dtcm.at_mram)/4 )
    __copy_table_end__ = .;
  } > MRAM

  .zero.table.at_mram : ALIGN(4)
  {
    __zero_table_start__ = .;
    LONG (ADDR(.bss.itcm))
    LONG (SIZEOF(.bss.itcm)/4)
    LONG (ADDR(.bss))
    LONG (SIZEOF(.bss)/4)
    __zero_table_end__ = .;
  } > MRAM

  .vectors.itcm (NOLOAD) : ALIGN(2048)
  {
    *(.bss.noinit.ram_vectors)
  } > ITCM

  .text.itcm.at_mram : ALIGN(8)
  {
    /* ITCM has non-startup code, CMSIS-DSP tables, audio record buffer and RAM vectors */
    *(.text*)
  } > ITCM AT> MRAM

  .data.dtcm.at_mram : ALIGN(8)
  {
    /* DTCM has all read-write data, including NPU activation buffer and audio processing buffers */
    *(vtable)
    *(.data)
    *(.data*)

    KEEP(*(.jcr*))

    . = ALIGN(8);
    *(.ARM.extab* .gnu.linkonce.armextab.*)
    . = ALIGN(8);
    __exidx_start = .;
    *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    __exidx_end = .;

  } > DTCM AT> MRAM

  .bss.itcm (NOLOAD) : ALIGN(8)
  {
    *(.bss.audio_rec)
  } > ITCM

  .bss (NOLOAD) : ALIGN(8)
  {
    __bss_start__ = .;
    *(.bss)
    *(.bss.*)
    *(COMMON)
    . = ALIGN(8);
    __bss_end__ = .;
  } > DTCM

  .heap (NOLOAD) : ALIGN(8)
  {
    __end__ = .;
    PROVIDE(end = .);
    . = . + __HEAP_SIZE;
    . = ALIGN(8);
    __HeapLimit = .;
  } > DTCM

  .stack (ORIGIN(DTCM) + LENGTH(DTCM) - __STACK_SIZE) (NOLOAD) : ALIGN(8)
  {
    __StackLimit = .;
    . = . + __STACK_SIZE;
    . = ALIGN(8);
    __StackTop = .;
  } > DTCM
  PROVIDE(__stack = __StackTop);

  .readonly.at_mram : ALIGN(8)
  {
    /* Use wildcards to mop up any read-only not directed to TCM */
    KEEP(*(.init))
    KEEP(*(.fini))

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

    *(.rodata*)

    *(nn_model)
    *(labels)
    *(ifm)

    KEEP(*(.eh_frame*))
  } > MRAM
}
