#!/usr/bin/env python3

#
# SPDX-FileCopyrightText: Copyright 2021-2024 Arm Limited and/or its affiliates <open-source-office@arm.com>
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

import argparse
import subprocess
import sys
import os
import platform
from pathlib import Path

def __print_arguments(args):
    if isinstance(args, list):
        print("$ " + " ".join(args))
    else:
        print(args)

def Popen(args, **kwargs):
    __print_arguments(args)
    return subprocess.Popen(args, **kwargs)

def call(args, **kwargs):
    __print_arguments(args)
    return subprocess.call(args, **kwargs)

def check_call(args, **kwargs):
    __print_arguments(args)
    return subprocess.check_call(args, **kwargs)

def check_output(args, **kwargs):
    __print_arguments(args)
    return subprocess.check_output(args, **kwargs)

def run_fvp(cmd):
    # Run FVP and tee output to console while scanning for exit tag
    ret = 1
    proc = Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    while True:
        line = proc.stdout.readline().decode()
        if not line:
                break

        if 'Application exit code: 0.' in line:
            ret = 0

        sys.stdout.write(line)
        sys.stdout.flush()

    return ret

def run_corstone_300(args):
    # FVP executable
    if not args.arch or args.arch == 'ethos-u55':
        fvp = 'FVP_Corstone_SSE-300_Ethos-U55'
    elif args.arch == 'ethos-u65':
        fvp = 'FVP_Corstone_SSE-300_Ethos-U65'
    else:
        raise 'Unsupported NPU arch'

    # FVP executable
    cmd = [fvp]

    # NPU configuration
    cmd += ['-C', 'ethosu.num_macs=' + str(args.macs)]

    # Output parameters
    cmd += ['-C', 'mps3_board.visualisation.disable-visualisation=1',
            '-C', 'mps3_board.telnetterminal0.start_telnet=0',
            '-C', 'mps3_board.uart0.out_file="-"',
            '-C', 'mps3_board.uart0.unbuffered_output=1',
            '-C', 'mps3_board.uart0.shutdown_on_eot=1']

    if args.tarmac:
        try:
            pvlib_home = Path(os.getenv('PVLIB_HOME'))
        except:
            raise Exception("Environment variable PVLIB_HOME not found. Needed to produce tarmac trace.")

        if sys.platform == 'linux':
            if platform.machine().lower() == 'arm64':
                tarmac_trace_plugin = pvlib_home / Path('plugins/Linux64_armv8l_GCC-9.3/TarmacTrace.so')
            else:
                tarmac_trace_plugin = pvlib_home / Path('plugins/Linux64_GCC-9.3/TarmacTrace.so')
        else:
            raise Exception("tarmac trace: This feature is not currently supported on" + sys.platform)

        if tarmac_trace_plugin.exists():
            print("Tarmac trace will be created");
            basename = [ e for e in args.args if e.endswith('.elf')][0][:-4]
            cmd += ['--plugin', str(tarmac_trace_plugin)]
            cmd += ['-C', f'TRACE.TarmacTrace.trace-file={basename}.trace']
        else:
            raise Exception("tarmac trace: Can't find TarmacTrace plugin in " + pvlib_home)

    cmd += args.args

    return run_fvp(cmd)

def run_corstone_310(args):
    # FVP executable
    if not args.arch or args.arch == 'ethos-u55':
        fvp = 'FVP_Corstone_SSE-310'
    elif args.arch == 'ethos-u65':
        fvp = 'FVP_Corstone_SSE-310_Ethos-U65'
    else:
        raise 'Unsupported NPU arch'
    cmd = ['FVP_Corstone_SSE-310']

    # NPU configuration
    cmd += ['-C', 'ethosu.num_macs=' + str(args.macs)]

    # Output parameters
    cmd += ['-C', 'mps3_board.visualisation.disable-visualisation=1',
            '-C', 'mps3_board.telnetterminal0.start_telnet=0',
            '-C', 'mps3_board.uart0.out_file="-"',
            '-C', 'mps3_board.uart0.unbuffered_output=1',
            '-C', 'mps3_board.uart0.shutdown_on_eot=1']

    cmd += args.args

    return run_fvp(cmd)

def run_corstone_320(args):
    if not args.arch or args.arch == 'ethos-u85':
        fvp = 'FVP_Corstone_SSE-320'
    else:
        raise 'Unsupported NPU arch'

    # FVP executable
    cmd = [fvp]

    # NPU configuration
    cmd += ['-C', 'mps4_board.subsystem.ethosu.num_macs=' + str(args.macs)]

    # Output parameters
    cmd += ['-C', 'mps4_board.visualisation.disable-visualisation=1',
            '-C', 'vis_hdlcd.disable_visualisation=1',
            '-C', 'mps4_board.telnetterminal0.start_telnet=0',
            '-C', 'mps4_board.uart0.out_file="-"',
            '-C', 'mps4_board.uart0.unbuffered_output=1',
            '-C', 'mps4_board.uart0.shutdown_on_eot=1']

    if args.tarmac:
        try:
            pvlib_home = Path(os.getenv('PVLIB_HOME'))
        except:
            raise Exception("Environment variable PVLIB_HOME not found. Needed to produce tarmac trace.")

        if sys.platform == 'linux':
            if platform.machine().lower() == 'arm64':
                tarmac_trace_plugin = pvlib_home / Path('plugins/Linux64_armv8l_GCC-10.3/TarmacTrace.so')
            else:
                tarmac_trace_plugin = pvlib_home / Path('plugins/Linux64_GCC-10.3/TarmacTrace.so')
        else:
            raise Exception("tarmac trace: This feature is not currently supported on" + sys.platform)

        if tarmac_trace_plugin.exists():
            print("Tarmac trace will be created");
            basename = [ e for e in args.args if e.endswith('.elf')][0][:-4]
            cmd += ['--plugin', str(tarmac_trace_plugin)]
            cmd += ['-C', f'TRACE.TarmacTrace.trace-file={basename}.trace']
        else:
            raise Exception("tarmac trace: Can't find TarmacTrace plugin in " + pvlib_home)

    cmd += args.args

    return run_fvp(cmd)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Run a test with given test command and test binary.')
    parser.add_argument('-t', '--target', choices=['corstone-300', 'corstone-310', 'corstone-320'], required=True, help='FVP target.')
    parser.add_argument('-a', '--arch', choices=['ethos-u55', 'ethos-u65', 'ethos-u85'], help='NPU architecture.')
    parser.add_argument('-m', '--macs', type=int, choices=[32, 64, 128, 256, 512, 1024, 2048], default=128, help='NPU number of MACs.')
    parser.add_argument('-c', '--tarmac', dest='tarmac', action='store_true', help='Collect tarmac traces when running FVP (experimental).')

    parser.add_argument('args', nargs='+', help='Arguments.')
    args = parser.parse_args()

    if args.target == 'corstone-300':
        sys.exit(run_corstone_300(args))
    elif args.target == 'corstone-310':
        sys.exit(run_corstone_310(args))
    elif args.target == 'corstone-320':
        sys.exit(run_corstone_320(args))
