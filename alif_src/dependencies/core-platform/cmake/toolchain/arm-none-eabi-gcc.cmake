#
# SPDX-FileCopyrightText: Copyright 2020-2022, 2024 Arm Limited and/or its affiliates <open-source-office@arm.com>
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the License); you may
# not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an AS IS BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

set(TARGET_CPU "cortex-m4" CACHE STRING "Target CPU")
string(TOLOWER ${TARGET_CPU} CMAKE_SYSTEM_PROCESSOR)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_C_COMPILER "arm-none-eabi-gcc")
set(CMAKE_CXX_COMPILER "arm-none-eabi-g++")
set(CMAKE_ASM_COMPILER "arm-none-eabi-gcc")
set(CMAKE_LINKER "arm-none-eabi-ld")
set(CMAKE_OBJCOPY "arm-none-eabi-objcopy")

set(CMAKE_EXECUTABLE_SUFFIX ".elf")
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Select C/C++ version
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Use cortex-m55 target for cortex-m52 due to missing compiler support in gcc version < 14
if (CMAKE_SYSTEM_PROCESSOR MATCHES "cortex-m52")
    message(WARNING "Compiling for cortex-m55 instead of cortex-m52 due to lack of support in gcc version < 14!")
    string(REPLACE "cortex-m52" "cortex-m55" CMAKE_SYSTEM_PROCESSOR ${CMAKE_SYSTEM_PROCESSOR})
endif()

# Compile options
add_compile_options(
    -mcpu=${CMAKE_SYSTEM_PROCESSOR}
    -mthumb
    "$<$<CONFIG:DEBUG>:-gdwarf-3>"
    "$<$<COMPILE_LANGUAGE:CXX>:-fno-unwind-tables;-fno-rtti;-fno-exceptions>"
    -fdata-sections
    -ffunction-sections)

# Compile defines
add_compile_definitions(
    "$<$<NOT:$<CONFIG:DEBUG>>:NDEBUG>")

# Link options
add_link_options(
    -mcpu=${CMAKE_SYSTEM_PROCESSOR}
    -mthumb
    --specs=nosys.specs)

# Set floating point unit
if(CMAKE_SYSTEM_PROCESSOR MATCHES "\\+fp")
    set(FLOAT hard)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "\\+nofp")
    set(FLOAT soft)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "cortex-m33(\\+|$)" OR
       CMAKE_SYSTEM_PROCESSOR MATCHES "cortex-m55(\\+|$)" OR
       CMAKE_SYSTEM_PROCESSOR MATCHES "cortex-m85(\\+|$)")
    set(FLOAT hard)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "cortex-m4(\\+|$)" OR
       CMAKE_SYSTEM_PROCESSOR MATCHES "cortex-m7(\\+|$)")
    set(FLOAT hard)
    set(FPU_CONFIG "fpv4-sp-d16")
    add_compile_options(-mfpu=${FPU_CONFIG})
    add_link_options(-mfpu=${FPU_CONFIG})
else()
    set(FLOAT soft)
endif()

if (FLOAT)
    add_compile_options(-mfloat-abi=${FLOAT})
    add_link_options(-mfloat-abi=${FLOAT})
endif()

add_link_options(LINKER:--nmagic,--gc-sections)

# Compilation warnings
add_compile_options(
    -Wall
    -Wextra

    -Wcast-align
    -Wdouble-promotion
    -Wformat
    -Wmissing-field-initializers
    -Wnull-dereference
    -Wredundant-decls
    -Wshadow
    -Wswitch
    -Wswitch-default
    -Wunused

    -Wno-redundant-decls

    -Wno-psabi
)
