/*
 *  Copyright (c) 2011 by Argonne National Laboratory
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to
 *  deal in the Software without restriction, including without limitation the
 *  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 *  sell copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *  IN THE SOFTWARE.
 */

/* A simple test of the proposed MPI_T_ interface (as "MPIX_T") that queries all
 * of the control variables exposed by the MPI implememtation and prints them to
 * stdout.
 *
 * Author: Dave Goodell <goodell@mcs.anl.gov.
 */

#include "mpi.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

int main(int argc, char **argv)
{
    int i;
    int num;
    int rank, size;
/*#define STR_SZ (15)*/
#define STR_SZ (50)
    int name_len = STR_SZ;
    char name[STR_SZ] = "";
    int desc_len = STR_SZ;
    char desc[STR_SZ] = "";
    int verb;
    MPI_Datatype dtype;
    int count;
    int bind;
    int scope;
    int provided;
    int initialize_mpi = 0;
    MPIX_T_cvar_handle handle;
    MPIX_T_enum enumtype;

    provided = 0xdeadbeef;
    MPIX_T_init_thread(MPI_THREAD_SINGLE, &provided);
    assert(provided != 0xdeadbeef);

    if (initialize_mpi) {
        MPI_Init(&argc, &argv);
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Comm_size(MPI_COMM_WORLD, &size);
    }

    num = 0xdeadbeef;
    MPIX_T_cvar_get_num(&num);
    printf("get_num=%d\n", num);
    assert(num != 0xdeadbeef);
    for (i = 0; i < num; ++i) {
        name_len = desc_len = STR_SZ;
        MPIX_T_cvar_get_info(i, name, &name_len, &verb, &dtype, &enumtype, desc, &desc_len, &bind, &scope);
        printf("index=%d\n", i);
        printf("--> name='%s' name_len=%d desc='%s' desc_len=%d\n", name, name_len, desc, desc_len);
        printf("--> verb=%d dtype=%#x bind=%d scope=%d\n", verb, dtype, bind, scope);

        MPIX_T_cvar_handle_alloc(i, NULL, &handle, &count);
        printf("--> handle allocated: handle=%p count=%d\n", handle, count);
        if (dtype == MPI_INT) {
            int val = 0xdeadbeef;
            MPIX_T_cvar_read(handle, &val);
            printf("--> val=%d\n", val);
            ++val;
            MPIX_T_cvar_write(handle, &val);
            val = 0xdeadbeef;
            MPIX_T_cvar_read(handle, &val);
            printf("--> incremented val=%d\n", val);
        }
        else if (dtype == MPI_DOUBLE) {
            double val = NAN;
            MPIX_T_cvar_read(handle, &val);
            printf("--> val=%f\n", val);
            val *= 2.0;
            MPIX_T_cvar_write(handle, &val);
            val = NAN;
            MPIX_T_cvar_read(handle, &val);
            printf("--> doubled val=%f\n", val);
        }
        else if (dtype == MPI_CHAR) {
            char *str = malloc(count+1);
            MPIX_T_cvar_read(handle, str);
            printf("--> str='%s'\n", str);
            /* just write the string back unmodified for now */
            MPIX_T_cvar_write(handle, str);
            MPIX_T_cvar_read(handle, str);
            printf("--> written-then-read str='%s'\n", str);
        }
        MPIX_T_cvar_handle_free(&handle);
        printf("\n");
    }

    if (initialize_mpi) {
        MPI_Finalize();
    }

    MPIX_T_finalize();

    return 0;
}

