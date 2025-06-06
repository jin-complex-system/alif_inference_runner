/*
 * SPDX-FileCopyrightText: Copyright 2021, 2024 Arm Limited and/or its
 * affiliates <open-source-office@arm.com>
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
#include "hal.h"                    /* Brings in platform definitions. */
#include "Model.hpp"            /* Model class for running inference. */
#include "UseCaseHandler.hpp"       /* Handlers for different user options. */
#include "UseCaseCommonUtils.hpp"   /* Utils functions. */
#include "log_macros.h"             /* Logging functions */
#include "BufAttributes.hpp"        /* Buffer attributes to be applied */

namespace arm {
namespace app {
    static uint8_t tensorArena[ACTIVATION_BUF_SZ] ACTIVATION_BUF_ATTRIBUTE;
    namespace inference_runner {
#if defined(DYNAMIC_MODEL_BASE) && defined(DYNAMIC_MODEL_SIZE)

static uint8_t* GetModelPointer()
{
    info("Model pointer: 0x%08x\n", DYNAMIC_MODEL_BASE);
    return reinterpret_cast<uint8_t*>(DYNAMIC_MODEL_BASE);
}

static size_t GetModelLen()
{
    /* TODO: Can we get the actual model size here somehow?
     * Currently we return the reserved space. It is possible to do
     * so by reading the memory pattern but it will not be reliable. */
    return static_cast<size_t>(DYNAMIC_MODEL_SIZE);
}

#else /* defined(DYNAMIC_MODEL_BASE) && defined(DYNAMIC_MODEL_SIZE) */

extern uint8_t* GetModelPointer();
extern size_t GetModelLen();

#endif /* defined(DYNAMIC_MODEL_BASE) && defined(DYNAMIC_MODEL_SIZE) */
    }  /* namespace inference_runner */
} /* namespace app */
} /* namespace arm */

#include "MicroMutableAllOpsResolver.hpp"

constexpr uint32_t MODEL_INPUT_SIZE = 68u * 384u;
constexpr uint32_t prefix_str_length = 0u;
constexpr uint32_t postfix_str_length = 1u;
constexpr uint32_t my_buffer_len = prefix_str_length + MODEL_INPUT_SIZE +  + postfix_str_length;
char my_buffer[my_buffer_len];

char input_buffer[MODEL_INPUT_SIZE];

void MainLoop()
{
    info("Main loop.\n");

    arm::app::Model model;  /* Model wrapper object. */

    /* Load the model. */
    if (!model.Init(arm::app::tensorArena,
                    sizeof(arm::app::tensorArena),
                    arm::app::inference_runner::GetModelPointer(),
                    arm::app::inference_runner::GetModelLen())) {
        printf_err("Failed to initialise model\n");
        return;
    }
    model.ShowModelInfoHandler();

    /* Instantiate application context. */
    arm::app::ApplicationContext caseContext;

    arm::app::Profiler profiler{"inference_runner"};
    caseContext.Set<arm::app::Profiler&>("profiler", profiler);
    caseContext.Set<arm::app::Model&>("model", model);

    uint32_t input_buffer_length = 0;
    while(true) {
        memset(my_buffer, 0, my_buffer_len);

        /// Read from user input
        hal_get_user_input(my_buffer, sizeof(my_buffer));

        // /// Dump user input
        // user_print(my_buffer);

        /// Parse the buffer
        switch(my_buffer[0]) {
            /// Clear input buffer
            case 'n':
                memset(input_buffer, 0, MODEL_INPUT_SIZE);
                input_buffer_length = 0;

                printf("Emptied\n");
                break;

            /// Insert and increment
            case 'i':
                if (input_buffer_length < MODEL_INPUT_SIZE) {
                    int8_t my_integer = atoi(&my_buffer[1]);

                    input_buffer[input_buffer_length++] = my_integer;
                    printf("%c\n", my_integer);
                }
                else {
                    printf("Full\n");
                }

                break;

            /// Execute
            case 'e':
                /// Number of executions
                int32_t num_iterations = atol(&my_buffer[1]);

                if (num_iterations <= 0) {
                    num_iterations = 1;
                }

                /// Copy input buffer to model
                TfLiteTensor* input_tensor = model.GetInputTensor(0);
                int8_t* input_data = tflite::GetTensorData<int8_t>(input_tensor);
                memcpy(input_data, input_buffer, input_tensor->bytes);

                if (!RunInference(model, profiler, num_iterations)) {
                    warn("Something went wrong with inferencing\n");
                }

                /// Retrieve output results (outer-most) and get the best result
                const auto output_index = model.GetNumOutputs() - 1;
                TfLiteTensor* output_tensor = model.GetOutputTensor(output_index);
                int8_t* output_data = tflite::GetTensorData<int8_t>(output_tensor);

                int8_t best_result = -1;
                uint16_t best_class = 0;
                
                for (uint32_t iterator = 0; iterator < output_tensor->bytes; iterator++) {
                    if (output_data[iterator] > best_result) {
                        best_result = output_data[iterator];
                        best_class = iterator;
                    }
                }
                printf("%u\n", best_class);
                break;
        }
    }
}
