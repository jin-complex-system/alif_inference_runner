/*
 * SPDX-FileCopyrightText: Copyright 2021-2022, 2024 Arm Limited and/or its
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
#include "UseCaseHandler.hpp"
#include "Classifier.hpp"
#include "ImageUtils.hpp"
#include "UseCaseCommonUtils.hpp"
#include "VisualWakeWordModel.hpp"
#include "VisualWakeWordProcessing.hpp"
#include "hal.h"
#include "log_macros.h"

namespace arm {
namespace app {

    /* Visual Wake Word inference handler. */
    bool ClassifyImageHandler(ApplicationContext& ctx)
    {
        auto& profiler = ctx.Get<Profiler&>("profiler");
        auto& model    = ctx.Get<Model&>("model");

        constexpr uint32_t dataPsnImgDownscaleFactor = 1;
        constexpr uint32_t dataPsnImgStartX          = 10;
        constexpr uint32_t dataPsnImgStartY          = 35;

        constexpr uint32_t dataPsnTxtInfStartX = 150;
        constexpr uint32_t dataPsnTxtInfStartY = 40;

        if (!model.IsInited()) {
            printf_err("Model is not initialised! Terminating processing.\n");
            return false;
        }

        TfLiteTensor* inputTensor  = model.GetInputTensor(0);
        TfLiteTensor* outputTensor = model.GetOutputTensor(0);
        if (!inputTensor->dims) {
            printf_err("Invalid input tensor dims\n");
            return false;
        } else if (inputTensor->dims->size < 4) {
            printf_err("Input tensor dimension should be = 4\n");
            return false;
        }

        /* Get input shape for displaying the image. */
        TfLiteIntArray* inputShape = model.GetInputShape(0);
        const uint32_t nCols = inputShape->data[arm::app::VisualWakeWordModel::ms_inputColsIdx];
        const uint32_t nRows = inputShape->data[arm::app::VisualWakeWordModel::ms_inputRowsIdx];
        if (arm::app::VisualWakeWordModel::ms_inputChannelsIdx >=
            static_cast<uint32_t>(inputShape->size)) {
            printf_err("Invalid channel index.\n");
            return false;
        }

        /* We expect RGB images to be provided. */
        const uint32_t displayChannels = 3;

        /* Set up pre and post-processing. */
        VisualWakeWordPreProcess preProcess = VisualWakeWordPreProcess(inputTensor);

        std::vector<ClassificationResult> results;
        VisualWakeWordPostProcess postProcess =
            VisualWakeWordPostProcess(outputTensor,
                                      ctx.Get<Classifier&>("classifier"),
                                      ctx.Get<std::vector<std::string>&>("labels"),
                                      results);
        hal_camera_init();
        auto bCamera = hal_camera_configure(nCols,
            nRows,
            HAL_CAMERA_MODE_SINGLE_FRAME,
            HAL_CAMERA_COLOUR_FORMAT_RGB888);
        if (!bCamera) {
            printf_err("Failed to configure camera.\n");
            return false;
        }

        while (true) {
            hal_lcd_clear(COLOR_BLACK);
            hal_camera_start();

            /* Strings for presentation/logging. */
            std::string str_inf{"Running inference... "};
            uint32_t capturedFrameSize = 0;
            const uint8_t* imgSrc = hal_camera_get_captured_frame(&capturedFrameSize);
            if (!imgSrc || !capturedFrameSize) {
                break;
            }

            /* Display this image on the LCD. */
            hal_lcd_display_image(imgSrc,
                                  nCols,
                                  nRows,
                                  displayChannels,
                                  dataPsnImgStartX,
                                  dataPsnImgStartY,
                                  dataPsnImgDownscaleFactor);

            /* Display message on the LCD - inference running. */
            hal_lcd_display_text(
                str_inf.c_str(), str_inf.size(), dataPsnTxtInfStartX, dataPsnTxtInfStartY, 0);

            const size_t imgSz =
                inputTensor->bytes < capturedFrameSize ? inputTensor->bytes : capturedFrameSize;

            /* Run the pre-processing, inference and post-processing. */
            if (!preProcess.DoPreProcess(imgSrc, imgSz)) {
                printf_err("Pre-processing failed.");
                return false;
            }

            if (!RunInference(model, profiler)) {
                printf_err("Inference failed.");
                return false;
            }

            if (!postProcess.DoPostProcess()) {
                printf_err("Post-processing failed.");
                return false;
            }

            /* Erase. */
            str_inf = std::string(str_inf.size(), ' ');
            hal_lcd_display_text(
                str_inf.c_str(), str_inf.size(), dataPsnTxtInfStartX, dataPsnTxtInfStartY, 0);

            /* Add results to context for access outside handler. */
            ctx.Set<std::vector<ClassificationResult>>("results", results);

#if VERIFY_TEST_OUTPUT
            arm::app::DumpTensor(outputTensor);
#endif /* VERIFY_TEST_OUTPUT */

            if (!PresentInferenceResult(results)) {
                return false;
            }
            profiler.PrintProfilingResult();

        }
        return true;
    }

} /* namespace app */
} /* namespace arm */
