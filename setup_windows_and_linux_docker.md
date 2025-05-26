# Setup Windows and Linux Docker

# Requirements

- Windows 11 PC
- Docker
- Ensemble Dev Kit
- `.tflite` model

# General steps
- [Setup Windows 11](#setup-windows-11)
- [Setup Ubuntu 22.04 Docker](#setup-ubuntu-2204-docker)
- [Build Evaluation Project](#build-evaluation-project)
- [Load Binary](#load-binary)

## Setup Windows 11
Modified from [Alif Security Toolkit User Guide v1.104.0](https://alifsemi.com/download/AUGD0005)

1. Download the [Alif Security Toolkit for Windows](https://alifsemi.com/download/APFW0001) from [Software Tools](https://alifsemi.com/support/software-tools/ensemble/)
- You may need to register an account and wait for account approval

2. Unzip the folder into `C:\app-release-exec>`

TODO: Add more steps

## Setup Ubuntu 22.04 Docker
Modified from [Evaluation Setup for Ubuntu](https://github.com/alifsemi/alif_ml-embedded-evaluation-kit/blob/main/ML_Embedded_Evaluation_Kit.md#ubuntu-setup)

1. Create a docker Linux container:
```bash
mkdir alif_stuff && cd alif_stuff
docker run -it -v "$(pwd):/mydir" --gpus all --network host -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix ubuntu:22.04
```

2. Update and install the necessary packages:
```bash
apt update && apt upgrade -y

# For pulling the project files
apt install git cmake xz-utils wget

# Install arm-related depencies
apt install libncurses5 libc6-i386 lib32gcc-s1 lib32stdc++6 # lib32z
apt install curl dos2unix # python-is-python

# Install python
apt-get install python3.10 python3.10-venv libpython3.10 libpython3.10-dev

# Verify python3 version 
python3 --version
```

3. From https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads, download for x86_64 Linux hosted crosstoolchain arm-gnu-toolchain:
```bash
mkdir arm_gnu_compiler && cd arm_gnu_compiler
wget https://developer.arm.com/-/media/Files/downloads/gnu/14.2.rel1/binrel/arm-gnu-toolchain-14.2.rel1-aarch64-arm-none-eabi.tar.xz
```

## Build Evaluation Project

4. Extract files from `.tar.xz` and place it into `/usr/local`:
```bash
tar xf arm-gnu-toolchain-14.2.rel1-x86_64-arm
-none-eabi.tar.xz -C . 
cd arm-gnu-toolchain-14.2.rel1-x86_64-arm-non
e-eabi/

# Probably a better way to copy over files
cp -r . /usr/local/.
```

5. Add compiler to path:
```
sh -c "echo export PATH=/usr/local/bin:$PATH > /etc/profile.d/arm-compiler.sh"
```

6. Clone the git repository to the directory
```bash
cd /mydir
git clone https://github.com/alifsemi/alif_ml-embedded-evaluation-kit.git
```

7. Initialise and update the required submodules
```bash
cd alif_ml-embedded-evaluation-kit
# git checkout 24.05 # Unsure if this is needed
git submodule update --init
```

8. Download and setup the required AI/ML resources
```bash
python3.10 set_up_default_resources.py --additional-ethos-u-config-name ethos-u55-128
```

9. Convert your `.tflite` model to target the NPU, and into a header file:
```bash
chmod +777 resources_downloaded/env/bin/activate
chmod +777 resources_downloaded/env/bin/vela

resources_downloaded/env/bin/activate

mkdir models && cd models
cp <target_directory>/CNN_litert.tflite . 
../resources_downloaded/env/bin/vela \
  --accelerator-config=ethos-u55-128 \
  --optimise Performance \
  --config ../scripts/vela/default_vela.ini \
  --memory-mode=Shared_Sram \
  --system-config=Ethos_U55_High_End_Embedded \
  --output-dir=. \
  CNN_litert.tflite

# Verify vela file exist before we convert into a header file
ls | grep CNN_litert_vela.tflite 
xxd -i CNN_litert_vela.tflite > CNN_litert_vela.h
```

10. Create a build directory for M55-HE core for a Generic Inference Runner
```bash
cd /mydir/alif_ml-embedded-evaluation-kit
mkdir build_he_infrun_CNN_vela && cd build_he_infrun_CNN_vela
```

11. Address known issues as described in [known_isses.md](/known_issues.md)

12. Configure build using `CMake` and build:
```bash
cmake .. \
  -DUSE_CASE_BUILD=inference_runner \
  -DTARGET_PLATFORM=ensemble \
  -DGLCD_UI=OFF \
  -DTARGET_SUBSYSTEM=RTSS-HE \
  -DTARGET_BOARD=DevKit \
  -DTARGET_REVISION=B \
  -DLINKER_SCRIPT_NAME=ensemble-RTSS-HE-infrun \
  -DCMAKE_TOOLCHAIN_FILE=scripts/cmake/toolchains/bare-metal-gcc.cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DLOG_LEVEL=LOG_LEVEL_INFO \
  -DCONSOLE_UART=2 \
  -Dinference_runner_MODEL_TFLITE_PATH=/mydir/alif_ml-embedded-evaluation-kit/models/CNN_litert_vela.tflite 
make -j16

# Convert your `.axf` file into a binary file
arm-none-eabi-objcopy -O binary bin/ethos-u-inference_runner.axf bin/CNN_litert_vela_inference_runner.bin

# Alternatively, you can just use mram.bin
# cp sectors/inference_runner/mram.bin CNN_litert_vela_inference_runner.bin
cp CNN_litert_vela_inference_runner.bin <path_to_windows_folder>
```

## Load Binary

1. Examine your binary folder:
```PowerShell
PS C:\Users\Parth\Desktop\bin> dir


    Directory: C:\Users\Parth\Desktop\bin


Mode                 LastWriteTime         Length Name
----                 -------------         ------ ----
d-----         08-May-25     16:55                sectors
-a----         08-May-25     16:51       14991772 ethos-u-inference_runner.axf
-a----         08-May-25     16:51        1558633 ethos-u-inference_runner.map


PS C:\Users\Parth\Desktop\bin> dir .\sectors\


    Directory: C:\Users\Parth\Desktop\bin\sectors


Mode                 LastWriteTime         Length Name
----                 -------------         ------ ----
d-----         08-May-25     17:16                inference_runner


PS C:\Users\Parth\Desktop\bin> dir .\sectors\inference_runner\


    Directory: C:\Users\Parth\Desktop\bin\sectors\inference_runner


Mode                 LastWriteTime         Length Name
----                 -------------         ------ ----
-a----         08-May-25     16:51        1001380 CNN_litert_vela_inference_runner.bin
-a----         08-May-25     16:51        1001380 mram.bin
```
2. Copy the binary into `C:\app-release-exec\build\images`:
```PowerShell
cp .\sectors\inference_runner\CNN_litert_vela_inference_runner.bin C:\app-release-exec\build\images\.
```

2. Add [`my_inference_runner.json`](helper_files/my_inference_runner.json) into `C:\app-release-exec\build\config`:
- Note that MRAM start address is at `0x80008000`

3. Navigate to `C:\app-release-exec`:
```PowerShell
cd C:\app-release-exec\
```

4. Run `app-gen-toc` to generate the package image, which will be written to `AppTocPackage.bin`:
```PowerShell
.\app-gen-toc.exe -f .\build\config\my_inference_runner.json
```

5. Prepare hardware by plugging in Ensemble Dev Kit (check device manager for COM port changes)

6. Write the application using SETOOLS:
```PowerShell
./app-write-mram -p
```

7. Connect to UART2 based on COM port (second port usually) on Device Manager
- Baud Rate: 115200
