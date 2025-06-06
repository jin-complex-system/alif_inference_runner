#!/bin/sh

extract_binary_from_model()
{
    ORIGINAL_MODEL=$1
    MODEL_DIRECTORY=$2

    cp ${MODEL_DIRECTORY}/${ORIGINAL_MODEL}.tflite models/.

    echo "Processing"
    echo ${ORIGINAL_MODEL}
    
    cd models
    ../resources_downloaded/env/bin/vela \
        --accelerator-config=ethos-u55-128 \
        --optimise Performance \
        --config ../scripts/vela/default_vela.ini \
        --memory-mode=Shared_Sram \
        --system-config=Ethos_U55_High_End_Embedded \
        --verbose-performance \
        --output-dir=. \
        ${ORIGINAL_MODEL}.tflite
    xxd -i ${ORIGINAL_MODEL}_vela.tflite > ${ORIGINAL_MODEL}_vela.h
    cd ..

    # build_and_make  "${ORIGINAL_MODEL}_vela"
    # build_and_make  "${ORIGINAL_MODEL}"

    echo "Done with"
    echo ${ORIGINAL_MODEL}
}

build_and_make()
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

hello_world()
{
    PRINT=$1
    echo $PRINT
}


###
# Main body
###

rm -rf build_he_infrun_*
rm -rf models
mkdir models

extract_binary_from_model "DTFT_Q" "../../my_models"
extract_binary_from_model "DTFT_SAC_Q" "../../my_models"
# extract_binary_from_model "TFT_Q" "../../my_models"
# extract_binary_from_model "FT_Q" "../../my_models"
extract_binary_from_model "CNN_litert" "../../my_models"
extract_binary_from_model "OB_model2_Q" "../../my_models"
extract_binary_from_model "model_orbw_19_Q" "../../my_models"
