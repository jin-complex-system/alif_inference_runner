#
# SPDX-FileCopyrightText: Copyright 2021, 2023-2024 Arm Limited and/or its affiliates <open-source-office@arm.com>
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

add_subdirectory(baremetal)

add_subdirectory(driver_unit_tests)

add_subdirectory(freertos)

add_subdirectory(hello_world)

add_subdirectory(threadx_demo)

add_subdirectory(message_handler_openamp)

if (CMAKE_CXX_COMPILER_ID STREQUAL "ARMClang")
    # Only armclang supported for now
    add_subdirectory(trustzone_inference)
endif()

message(STATUS "*******************************************************")
message(STATUS "PROJECT_NAME                           : ${PROJECT_NAME}")
message(STATUS "TR_ARENA_SIZE                          : ${TR_ARENA_SIZE}")
message(STATUS "MESSAGE_HANDLER_ARENA_SIZE             : ${MESSAGE_HANDLER_ARENA_SIZE}")
message(STATUS "*******************************************************")
