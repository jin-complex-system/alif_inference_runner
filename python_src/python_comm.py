import serial
import os
import numpy as np
import glob
import time

def parse_directory(target_directory):
    P_FILE_EXT = "P"

    samples = list()
    labels = list()

    for file_path in glob.glob(os.path.join(target_directory, f'*.{P_FILE_EXT}')):
        sample, label = parse_input_file(file_path)

        samples.append(sample)
        labels.append(label)
    return samples, labels    

def parse_input_file(target_filepath):
    # Retrieve label from original filename
    filename = os.path.basename(target_filepath)
    true_label_str_in_hex = filename.split("_")[1].split("~")[0] # Need to consider tilda
    true_label = int(true_label_str_in_hex.strip(), 16)

    sample = np.fromfile(target_filepath, dtype=np.int8)
    assert(len(sample) > 0)

    return sample, true_label

def send_and_listen_to_response(
        serialPort,
        message,
        expected_response_time):
    """
    """

    serialPort.reset_output_buffer()
    serialPort.reset_input_buffer()

    # Write then sleep
    numBytes_written = serialPort.write(message)
    serialPort.flush()
    time.sleep(expected_response_time)

    serialString = serialPort.read_all()

    # Discard sent string
    serialString = serialString[numBytes_written:len(serialString)]

    serialPort.reset_output_buffer()
    serialPort.reset_input_buffer()

    return serialString

def extract_data(response_msg):
    # Discard last character
    data_message = response_msg[0:len(response_msg) - 1]

    return data_message

def send_user_input(
        serialPort,
        np_array,
        write_timeout_seconds,
        wait_for_execution_timeout,
        num_iterations,
        debug=False):
    
    # Clear buffer
    clear_buffer_response = send_and_listen_to_response(
        serialPort=serialPort,
        message=bytearray("n \n", "utf-8"),
        expected_response_time=write_timeout_seconds,
    )

    # For windows, discard the first byte
    clear_buffer_response = clear_buffer_response[1:len(clear_buffer_response)]
    clear_buffer_response = extract_data(response_msg=clear_buffer_response)

    if (debug):
        print("Clear Buffer Response: {}".format(clear_buffer_response))

    for data_element in np_array:
        actual_data_line = "i {}\n".format(data_element)
        data_response = send_and_listen_to_response(
            serialPort=serialPort,
            message=bytearray(actual_data_line, "utf-8"),
            expected_response_time=write_timeout_seconds,
        )
        # # For windows, discard the first byte
        # data_response = data_response[1:len(data_response)]
        # data_message = extract_data(response_msg=data_response)

        # if (debug):
        #     print("Data response: {}, Data Message: {}, actual data: {}".format(
        #         data_response,
        #         data_message,
        #         data_element))
        #     assert(data_message == data_element.tobytes())

    # Execute
    inference_response = send_and_listen_to_response(
        serialPort=serialPort,
        message=bytearray("e {}\n".format(num_iterations), "utf-8"),
        expected_response_time=wait_for_execution_timeout,
    )

    # For windows, discard the first byte
    inference_response = inference_response[1:len(inference_response)]
    inference_response = extract_data(response_msg=inference_response)
    prediction = np.uint8(int(inference_response))

    if (debug):
        print("Inference Response: {}, prediction: {}".format(inference_response, prediction))
    
    return prediction

def execute_test(
        target_directory,
        target_prefix,
        debug=False):
    samples, y_test = parse_directory("INPUT")
    y_test = np.array(y_test, dtype=np.uint8)
    assert(len(samples) == len(y_test))

    y_test.tofile(os.path.join(
        target_directory,
        "{}_y_test.P".format(target_prefix)))

    y_pred = np.zeros(shape=0, dtype=np.uint8)
    
    SERIAL_TIMEOUT_SECONDS = 2

    serialPort = serial.Serial(
        port="COM9",
        baudrate=115200,
        bytesize=serial.EIGHTBITS,
        timeout=SERIAL_TIMEOUT_SECONDS,
        stopbits=serial.STOPBITS_ONE,
        parity=serial.PARITY_NONE
    )

    # Reset serial port buffers
    serialPort.read_all()
    serialPort.reset_output_buffer()
    serialPort.reset_input_buffer()
    time.sleep(SERIAL_TIMEOUT_SECONDS * 2)

    # Test with helloworld
    # while True:
    #     actual_data = "helloworld"
    #     actual_data_line = "{}\n".format(actual_data)

    #     response = send_and_listen_to_response(
    #         serialPort=serialPort,
    #         message=bytearray(actual_data_line, "utf-8"),
    #         expected_write_timeout_seconds=0.5,
    #     )

    #     # For windows, discard the first byte
    #     response = response[1:len(response)]
    #     print("Response: {}".format(response))

    #     # look for 'USER_PRINT'
    #     if b"USER_PRINT" in response:
    #         data_message = response.split(b'-')[1].split(b'\n')[0]
    #         print(data_message)

    #         assert(data_message == bytearray(actual_data, "utf-8"))
    #     time.sleep(SERIAL_TIMEOUT_SECONDS * 2)

    print("Begin loading {} inputs into embedded model".format(len(samples)))
    for sample_iterator in range(0, len(samples)):
        sample = samples[sample_iterator]

        prediction = send_user_input(
            serialPort=serialPort,
            np_array=sample,
            write_timeout_seconds=0.01,
            wait_for_execution_timeout=0.1,
            num_iterations=1,
            debug=debug,
        )
        y_pred = np.append(y_pred, prediction)
        y_pred.tofile(os.path.join(
            target_directory,
            "{}_y_pred.P".format(target_prefix)))
        
        if (debug):
            print("Iterator: {}, Label: {}, prediction: {}".format(
                sample_iterator,
                y_test[sample_iterator],
                y_pred[sample_iterator]))

    print(y_pred)

    serialPort.close()
    print("Done with execute_test()")

def _main():
    execute_test(
        target_directory="test",
        target_prefix="test")

if __name__ == '__main__':
    _main()
