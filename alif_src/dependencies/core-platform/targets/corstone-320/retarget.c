/*
 * SPDX-FileCopyrightText: Copyright 2019-2024 Arm Limited and/or its affiliates <open-source-office@arm.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(__ARMCC_VERSION)
#include <sys/stat.h>
#endif
#include <time.h>

#include "uart_stdout.h"

// armclang retargeting
#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6100100)
#include <rt_misc.h>
#include <rt_sys.h>

/* Standard IO device handles. */
#define STDIN  0x8001
#define STDOUT 0x8002
#define STDERR 0x8003

#define RETARGET(fun)  _sys##fun
#define IO_OUTPUT(len) 0

#else
/*
 * This type is used by the _ I/O functions to denote an open
 * file.
 */
typedef int FILEHANDLE;

/*
 * Open a file. May return -1 if the file failed to open.
 */
extern FILEHANDLE _open(const char * /*name*/, int /*openmode*/);

/* Standard IO device handles. */
#define STDIN  0x00
#define STDOUT 0x01
#define STDERR 0x02

#define RETARGET(fun)  fun
#define IO_OUTPUT(len) len

#endif

/* Standard IO device name defines. */
const char __stdin_name[] __attribute__((aligned(4)))  = "STDIN";
const char __stdout_name[] __attribute__((aligned(4))) = "STDOUT";
const char __stderr_name[] __attribute__((aligned(4))) = "STDERR";

void _ttywrch(int ch) {
    (void)fputc(ch, stdout);
}

FILEHANDLE RETARGET(_open)(const char *name, int openmode) {
    (void)openmode;

    if (strcmp(name, __stdin_name) == 0) {
        return (STDIN);
    }

    if (strcmp(name, __stdout_name) == 0) {
        return (STDOUT);
    }

    if (strcmp(name, __stderr_name) == 0) {
        return (STDERR);
    }

    return -1;
}

int RETARGET(_write)(FILEHANDLE fh, const unsigned char *buf, unsigned int len, int mode) {
    (void)mode;

    switch (fh) {
    case STDOUT:
    case STDERR: {
        int c;
        unsigned int i;

        for (i = 0; i < len; i++) {
            c = fputc(buf[i], stdout);
            if (c == EOF) {
                return EOF;
            }
        }

        return IO_OUTPUT(len);
    }
    default:
        return EOF;
    }
}

int RETARGET(_read)(FILEHANDLE fh, unsigned char *buf, unsigned int len, int mode) {
    (void)mode;

    switch (fh) {
    case STDIN: {
        int c;
        unsigned int i;

        for (i = 0; i < len; i++) {
            c = fgetc(stdin);
            if (c == EOF) {
                return EOF;
            }

            buf[i] = (unsigned char)c;
        }

        return IO_OUTPUT(len);
    }
    default:
        return EOF;
    }
}

int RETARGET(_istty)(FILEHANDLE fh) {
    switch (fh) {
    case STDIN:
    case STDOUT:
    case STDERR:
        return 1;
    default:
        return 0;
    }
}

int RETARGET(_close)(FILEHANDLE fh) {
    if (RETARGET(_istty(fh))) {
        return 0;
    }

    return -1;
}

int RETARGET(_seek)(FILEHANDLE fh, long pos) {
    (void)fh;
    (void)pos;

    return -1;
}

int RETARGET(_ensure)(FILEHANDLE fh) {
    (void)fh;

    return -1;
}

long RETARGET(_flen)(FILEHANDLE fh) {
    if (RETARGET(_istty)(fh)) {
        return 0;
    }

    return -1;
}

#if __ARMCLIB_VERSION >= 6190004
int RETARGET(_tmpnam2)(char *name, int sig, unsigned maxlen) {
    (void)name;
    (void)sig;
    (void)maxlen;

    return -1;
}
#else
int RETARGET(_tmpnam)(char *name, int sig, unsigned maxlen) {
    (void)name;
    (void)sig;
    (void)maxlen;

    return 1;
}
#endif

char *RETARGET(_command_string)(char *cmd, int len) {
    (void)len;

    return cmd;
}

void RETARGET(_exit)(int return_code) {
    char exit_code_buffer[64] = {0};
    const char *p             = exit_code_buffer;

    /* Print out the exit code on the uart so any reader know how we exit. */
    /* By appending 0x04, ASCII for end-of-transmission the FVP model exits,
     * if the configuration parameter shutdown_on_eot on the uart is enabled.
     */

    snprintf(exit_code_buffer,
             sizeof(exit_code_buffer),
             "Application exit code: %d.\n" // Let the readers know how we exit
             "\04\n",                       // end-of-transmission
             return_code);

    while (*p != '\0') {
        UartPutc(*p++);
    }

    while (1) {}
}

#if !defined(__ARMCC_VERSION)
int RETARGET(_fstat)(FILEHANDLE fh, struct stat *st) {
    (void)fh;
    (void)st;

    return -1;
}

int RETARGET(_getpid)(void) {
    return -1;
}

int RETARGET(_isatty)(FILEHANDLE fh) {
    (void)fh;

    return 0;
}

int RETARGET(_kill)(int pid, int sig) {
    (void)pid;
    (void)sig;

    return -1;
}

int RETARGET(_lseek)(FILEHANDLE fh, int ptr, int dir) {
    (void)fh;
    (void)ptr;
    (void)dir;

    return -1;
}
#endif

int system(const char *cmd) {
    (void)cmd;

    return 0;
}

time_t time(time_t *timer) {
    (void)timer;
    return (time_t)-1;
}

void _clock_init(void) {}

clock_t clock(void) {
    return (clock_t)-1;
}

int remove(const char *arg) {
    (void)arg;
    return 0;
}

int rename(const char *oldn, const char *newn) {
    (void)oldn;
    (void)newn;
    return 0;
}

int fputc(int ch, FILE *f) {
    (void)(f);
    return UartPutc(ch);
}

int fgetc(FILE *f) {
    (void)f;
    return UartPutc(UartGetc());
}

#ifndef ferror
/* arm-none-eabi-gcc with newlib uses a define for ferror */
int ferror(FILE *f) {
    (void)f;
    return EOF;
}
#endif
