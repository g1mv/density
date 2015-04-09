/*
 * Centaurean Density benchmark
 *
 * Copyright (c) 2015, Guillaume Voirin
 * All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * 6/04/15 0:11
 */

#ifndef DENSITY_BENCHMARK_H
#define DENSITY_BENCHMARK_H

#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>

#if defined(_WIN64) || defined(_WIN32)
#include <windows.h>
#include <time.h>

#define RUSAGE_SELF     0
#define RUSAGE_THREAD   1

struct rusage {
    struct timeval ru_utime; /* user CPU time used */
    struct timeval ru_stime; /* system CPU time used */
    long   ru_maxrss;        /* maximum resident set size */
    long   ru_ixrss;         /* integral shared memory size */
    long   ru_idrss;         /* integral unshared data size */
    long   ru_isrss;         /* integral unshared stack size */
    long   ru_minflt;        /* page reclaims (soft page faults) */
    long   ru_majflt;        /* page faults (hard page faults) */
    long   ru_nswap;         /* swaps */
    long   ru_inblock;       /* block input operations */
    long   ru_oublock;       /* block output operations */
    long   ru_msgsnd;        /* IPC messages sent */
    long   ru_msgrcv;        /* IPC messages received */
    long   ru_nsignals;      /* signals received */
    long   ru_nvcsw;         /* voluntary context switches */
    long   ru_nivcsw;        /* involuntary context switches */
};
#else
#include <sys/resource.h>

#define DENSITY_BENCHMARK_ALLOW_ANSI_ESCAPE_SEQUENCES
#endif

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define DENSITY_BENCHMARK_ENDIAN_STRING           "Little"
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define DENSITY_BENCHMARK_ENDIAN_STRING           "Big"
#endif

#ifdef __clang__
#define DENSITY_BENCHMARK_COMPILER                "Clang %d.%d.%d"
#define DENSITY_BENCHMARK_COMPILER_VERSION        __clang_major__, __clang_minor__, __clang_patchlevel__
#elif defined(__GNUC__)
#define DENSITY_BENCHMARK_COMPILER                "GCC %d.%d.%d"
#define DENSITY_BENCHMARK_COMPILER_VERSION        __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__
#else
#define DENSITY_BENCHMARK_COMPILER                "Unknown"
#define DENSITY_BENCHMARK_COMPILER_VERSION
#endif

#if defined(_WIN64) || defined(_WIN32)
#define DENSITY_BENCHMARK_PLATFORM_STRING         "Microsoft Windows"
#elif defined(__APPLE__)
#include "TargetConditionals.h"
#if TARGET_IPHONE_SIMULATOR
#define DENSITY_BENCHMARK_PLATFORM_STRING         "iOS Simulator"
#elif TARGET_OS_IPHONE
#define DENSITY_BENCHMARK_PLATFORM_STRING         "iOS"
#elif TARGET_OS_MAC
#define DENSITY_BENCHMARK_PLATFORM_STRING         "Mac OS/X"
#else
#define DENSITY_BENCHMARK_PLATFORM_STRING         "an unknown Apple platform"
#endif
#elif defined(__FreeBSD__)
#define DENSITY_BENCHMARK_PLATFORM_STRING         "FreeBSD"
#elif defined(__linux__)
#define DENSITY_BENCHMARK_PLATFORM_STRING         "GNU/Linux"
#elif defined(__unix__)
#define DENSITY_BENCHMARK_PLATFORM_STRING         "Unix"
#elif defined(__posix__)
#define DENSITY_BENCHMARK_PLATFORM_STRING         "Posix"
#else
#define DENSITY_BENCHMARK_PLATFORM_STRING         "an unknown platform"
#endif

#include "../../density_api.h"
#include "../../buffer.c"
#include "../../stream.c"
#include "../../memory_teleport.c"
#include "../../memory_location.c"
#include "../../kernel_chameleon_encode.c"
#include "../../kernel_cheetah_encode.c"
#include "../../kernel_lion_encode.c"
#include "../../kernel_chameleon_decode.c"
#include "../../kernel_cheetah_decode.c"
#include "../../kernel_lion_decode.c"
#include "../../kernel_chameleon_dictionary.c"
#include "../../kernel_cheetah_dictionary.c"
#include "../../kernel_lion_dictionary.c"
#include "../../kernel_lion_form_model.c"
#include "../../block_encode.c"
#include "../../block_decode.c"
#include "../../main_encode.c"
#include "../../main_decode.c"
#include "../../main_header.c"
#include "../../main_footer.c"
#include "../../block_header.c"
#include "../../block_footer.c"
#include "../../block_mode_marker.c"
#include "../../spookyhash/src/context.c"
#include "../../spookyhash/src/spookyhash.c"

#define DENSITY_CHRONO_MICROSECONDS         1000000.0
#define DENSITY_ESCAPE_CHARACTER            ((char)27)

#ifdef DENSITY_BENCHMARK_ALLOW_ANSI_ESCAPE_SEQUENCES
#define DENSITY_BENCHMARK_BOLD(op)          printf("%c[1m", DENSITY_ESCAPE_CHARACTER);\
                                            op;\
                                            printf("%c[0m", DENSITY_ESCAPE_CHARACTER);

#define DENSITY_BENCHMARK_BLUE(op)          printf("%c[0;34m", DENSITY_ESCAPE_CHARACTER);\
                                            op;\
                                            printf("%c[0m", DENSITY_ESCAPE_CHARACTER);

#define DENSITY_BENCHMARK_RED(op)           printf("%c[0;31m", DENSITY_ESCAPE_CHARACTER);\
                                            op;\
                                            printf("%c[0m", DENSITY_ESCAPE_CHARACTER);
#else
#define DENSITY_BENCHMARK_BOLD(op)          op;
#define DENSITY_BENCHMARK_BLUE(op)          op;
#define DENSITY_BENCHMARK_RED(op)           op;
#endif

#define DENSITY_BENCHMARK_UNDERLINE(n)      printf("\n");\
                                            for(int i = 0; i < n; i++)  printf("=");

#define DENSITY_BENCHMARK_ERROR(op, issue)  DENSITY_BENCHMARK_RED(DENSITY_BENCHMARK_BOLD(printf("\nAn error has occured !\n")));\
                                            op;\
                                            printf("\n");\
                                            if(issue) {\
                                                printf("Please open an issue at <https://github.com/centaurean/density/issues>, with your platform information and any relevant file.\n");\
                                                DENSITY_BENCHMARK_BOLD(printf("Thank you !\n"));\
                                            }\
                                            fflush(stdout);\
                                            exit(0);

#endif