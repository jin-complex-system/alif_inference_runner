#!/bin/sh

extract_binary_from_model()
{
    ORIGINAL_MODEL=$1
    MODEL_DIRECTORY=$2

    cp ${MODEL_DIRECTORY}/${ORIGINAL_MODEL}.tflite models/.

    echo "Processing"
    echo ${ORIGINAL_MODEL}
    
    cd models

    # Original model
    xxd -i ${ORIGINAL_MODEL}.tflite > ${ORIGINAL_MODEL}_HE.h

    # High-Efficiency Core
    HE_SYSTEM_CONFIG="RTSS_HE_SRAM_MRAM"
    ../resources_downloaded/env/bin/vela \
        --accelerator-config=ethos-u55-128 \
        --optimise Performance \
        --config ../scripts/vela/ensemble_vela.ini \
        --memory-mode=Shared_Sram \
        --system-config=${HE_SYSTEM_CONFIG} \
        --output-dir=. \
        ${ORIGINAL_MODEL}.tflite
    mv ${ORIGINAL_MODEL}_vela.tflite ${ORIGINAL_MODEL}_HE_vela.tflite
    xxd -i ${ORIGINAL_MODEL}_HE_vela.tflite > ${ORIGINAL_MODEL}_HE_vela.h

    # High-Performance Core
    HP_SYSTEM_CONFIG="RTSS_HP_SRAM_MRAM"
    ../resources_downloaded/env/bin/vela \
        --accelerator-config=ethos-u55-256 \
        --optimise Performance \
        --config ../scripts/vela/ensemble_vela.ini \
        --memory-mode=Shared_Sram \
        --system-config=${HP_SYSTEM_CONFIG} \
        --output-dir=. \
        ${ORIGINAL_MODEL}.tflite
    mv ${ORIGINAL_MODEL}_vela.tflite ${ORIGINAL_MODEL}_HP_vela.tflite
    xxd -i ${ORIGINAL_MODEL}_HP_vela.tflite > ${ORIGINAL_MODEL}_HP_vela.h

    cd ..
    build_and_make_he  "${ORIGINAL_MODEL}_HE_vela"
    build_and_make_hp  "${ORIGINAL_MODEL}_HP_vela"

    # No NPU on HE
    build_and_make_he  "${ORIGINAL_MODEL}"

    echo "Done with"
    echo ${ORIGINAL_MODEL}
}

build_and_make_he()
{
    MODEL_NAME=$1

    mkdir build_he_infrun_${MODEL_NAME}
    cd build_he_infrun_${MODEL_NAME}
    cmake .. \
        -DUSE_CASE_BUILD=inference_runner \
        -DTARGET_PLATFORM=ensemble \
        -DGLCD_UI=OFF \
        -DTARGET_SUBSYSTEM=RTSS-HE \
        -DTARGET_BOARD=DevKit \
        -DTARGET_REVISION=B \
        -DLINKER_SCRIPT_NAME=ensemble-RTSS-HE-infrun \
        -DCMAKE_TOOLCHAIN_FILE=../scripts/cmake/toolchains/bare-metal-gcc.cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DLOG_LEVEL=LOG_LEVEL_INFO \
        -DCONSOLE_UART=2 \
        -Dinference_runner_MODEL_TFLITE_PATH="models/${MODEL_NAME}.tflite "
    make -j16

    arm-none-eabi-objcopy -O binary bin/ethos-u-inference_runner.axf "bin/${MODEL_NAME}_inference_runner.bin"
    cd ..

    cp build_he_infrun_${MODEL_NAME}/bin/${MODEL_NAME}_inference_runner.bin models/.
}

build_and_make_hp()
{
    MODEL_NAME=$1

    mkdir build_hp_infrun_${MODEL_NAME}
    cd build_hp_infrun_${MODEL_NAME}
    cmake .. \
        -DUSE_CASE_BUILD=inference_runner \
        -DTARGET_PLATFORM=ensemble \
        -DGLCD_UI=OFF \
        -DTARGET_SUBSYSTEM=RTSS-HP \
        -DTARGET_BOARD=DevKit \
        -DTARGET_REVISION=B \
        -DLINKER_SCRIPT_NAME=ensemble-RTSS-HP-infrun \
        -DCMAKE_TOOLCHAIN_FILE=../scripts/cmake/toolchains/bare-metal-gcc.cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DLOG_LEVEL=LOG_LEVEL_INFO \
        -DCONSOLE_UART=2 \
        -Dinference_runner_MODEL_TFLITE_PATH="models/${MODEL_NAME}.tflite "
    make -j16

    arm-none-eabi-objcopy -O binary bin/ethos-u-inference_runner.axf "bin/${MODEL_NAME}_inference_runner.bin"
    cd ..

    cp build_hp_infrun_${MODEL_NAME}/bin/${MODEL_NAME}_inference_runner.bin models/.
}


hello_world()
{
    PRINT=$1
    echo $PRINT
}

###
# Main body
###

rm -rf build_he_infrun_*
rm -rf build_hp_infrun_*
rm -rf models
mkdir models

extract_binary_from_model "DTFT_Q" "../../my_models"
extract_binary_from_model "DTFT_SAC_Q" "../../my_models"
extract_binary_from_model "TFT_Q" "../../my_models"
extract_binary_from_model "FT_Q" "../../my_models"
extract_binary_from_model "CNN_litert" "../../my_models"
extract_binary_from_model "OB_model2_Q" "../../my_models"
extract_binary_from_model "model_us_Q" "../../my_models"
extract_binary_from_model "model_orbw_19_Q" "../../my_models"
