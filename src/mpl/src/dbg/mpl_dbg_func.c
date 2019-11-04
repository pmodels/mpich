/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * This file provides functions that enables function logging enabled by * gcc -finstrument-function.
 */

#include "mpl.h"

#ifdef MPL_HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef MPL_USE_DBG_LOGGING

#define MPL_DBG_MSG_ENTER(_func, _addr) \
    { \
        if ((MPL_DBG_ROUTINE_ENTER & MPL_dbg_active_classes) && MPL_DBG_VERBOSE <= MPL_dbg_max_level) { \
            MPL_dbg_outevent(NULL, 0, MPL_DBG_ROUTINE_ENTER, 4, "[ADDR:%p] Entering [FUNCTION:%p]", _addr, _func); \
        } \
    }

#define MPL_DBG_MSG_EXIT(_func, _addr) \
    { \
        if ((MPL_DBG_ROUTINE_EXIT & MPL_dbg_active_classes) && MPL_DBG_VERBOSE <= MPL_dbg_max_level) { \
            MPL_dbg_outevent(NULL, 0, MPL_DBG_ROUTINE_EXIT, 4, "[ADDR:%p] Leaving [FUNCTION:%p]", _addr, _func); \
        } \
    }

#else

#define MPL_DBG_MSG_ENTER(_func, _addr)
#define MPL_DBG_MSG_EXIT(_func, _addr)

#endif

static void *addr_base;
static void *addr_ext;

void __cyg_profile_func_enter(void *fn, void *addr) MPL_NOINSTRUMENT;
void __cyg_profile_func_exit(void *fn, void *addr) MPL_NOINSTRUMENT;
static void get_map(void) MPL_NOINSTRUMENT;

void __cyg_profile_func_enter(void *fn, void *addr)
{
    MPL_DBG_MSG_ENTER(fn, addr);
}

void __cyg_profile_func_exit(void *fn, void *addr)
{
    MPL_DBG_MSG_EXIT(fn, addr);
}

void mpl_dbg_init_instrument_function()
{
    get_map();
}

/* **** internal functions **** */
static char binary_pathname[1024];

/* The addresses are shifted by potentially random base address.
 * get_map() retrieves that base address, as well as the binary
 * path. This is needed for translating the call-site and function
 * address back to file lcoation and function names.
 *
 * It is possible to do the translation within the logging calls.
 * However, to keep it simple and overhead low, we log the address
 * directly, and rely on utility script to translate the log (and
 * pretty printing.
 */
void get_map()
{
    char s_map[20];
    pid_t pid = getpid();
    snprintf(s_map, 20, "/proc/%d/maps", pid);

    FILE *file_in = fopen(s_map, "rb");
    if (file_in == NULL) {
        fprintf(stderr, "Can't open %s\n", s_map);
        exit(-1);
    } else {
        char line[1024];
        while (fgets(line, 1024, file_in)) {
            /* address           perms offset  dev   inode   pathname
             * 7fc9afdc4000-7fc9b022c000 r-xp 00000000 08:03 5383101
             *     /home/.../_inst/lib/libmpi.so.0.0.0
             */
            char *s = line;
            char *s2;
            uintptr_t a = strtoll(s, &s2, 16);
            uintptr_t b = strtoll(s2 + 1, &s2, 16);
            uintptr_t t = (uintptr_t) get_map;
            if (t >= a && t <= b) {
                MPL_DBG_MSG(MPL_DBG_ALL, TERSE, line);
                addr_base = (void *) a;
                addr_ext = (void *) b;
                /* seek /path/to/name */
                s = s2 + 1;
                while (*s && *s != '/') {
                    s++;
                }
                strncpy(binary_pathname, s, 1024);
                /* chop \n */
                break;
            }
        }
        fclose(file_in);
    }
}
