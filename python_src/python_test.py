import python_comm as comm

def test_loop(
        target_prefix,
        debug):
    import os
    import time
    print("Executing test for {}".format(target_prefix))

    APP_RELEASE_EXEC_DIRECTORY = "C:\\app-release-exec"
    CURRENT_DIRECTORY = os.getcwd()

    target_json_file = "{}\\build\\config\\{}.json".format(APP_RELEASE_EXEC_DIRECTORY, target_prefix)
    APP_WRITE_MRAM_CMD = "{}\\app-write-mram -p".format(APP_RELEASE_EXEC_DIRECTORY)
    APP_GEN_TOC_EXE_CMD = "{}\\app-gen-toc.exe -f {}".format(APP_RELEASE_EXEC_DIRECTORY, target_json_file)

    # Program target embedded device
    os.chdir(APP_RELEASE_EXEC_DIRECTORY)
    os.system(APP_GEN_TOC_EXE_CMD)
    time.sleep(1)
    os.system(APP_WRITE_MRAM_CMD)
    time.sleep(2)

    # Execute test
    # make sure we are back in the current directory
    os.chdir(CURRENT_DIRECTORY)
    os.makedirs(target_prefix)
    comm.execute_test(
        target_directory=target_prefix,
        target_prefix=target_prefix,
        debug=debug)
    time.sleep(1)

def main():
    TARGET = [
        'model_orbw_19_Q',
        # 'OB_model2_Q',
        # 'DTFT_SAC_Q',
        # 'CNN_litert',
        # 'DTFT_Q',
        # 'FT_Q',
        # 'TFT_Q',
    ]

    TARGET_POSTFIX = [
        '_vela',
        # '',
    ]

    for postfix in TARGET_POSTFIX:
        for target in TARGET:
            target_prefix = "{}{}".format(target, postfix)

            test_loop(
                target_prefix=target_prefix,
                debug=True)

if __name__ == '__main__':
    main()
