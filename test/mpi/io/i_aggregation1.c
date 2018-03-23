/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* Test case from John Bent (ROMIO req #835)
 * Aggregation code was not handling certain access patterns when collective
 * buffering forced */

/* Uses nonblocking collective I/O.*/

#include <unistd.h>
#include <stdlib.h>
#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include "mpitest.h"

#define NUM_OBJS 4
#define OBJ_SIZE 1048576

extern char *optarg;
extern int optind, opterr, optopt;


char *prog = NULL;
int debug = 0;

static void Usage(int line)
{
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        fprintf(stderr,
                "Usage (line %d): %s [-d] [-h] -f filename\n"
                "\t-d for debugging\n"
                "\t-h to turn on the hints to force collective aggregation\n", line, prog);
    }
    exit(0);
}

static void fatal_error(int mpi_ret, MPI_Status * mpi_stat, const char *msg)
{
    fprintf(stderr, "Fatal error %s: %d\n", msg, mpi_ret);
    MPI_Abort(MPI_COMM_WORLD, -1);
}

static void print_hints(int rank, MPI_File * mfh)
{
    MPI_Info info;
    int nkeys;
    int i, dummy_int;
    char key[1024];
    char value[1024];

    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == 0) {
        MPI_File_get_info(*mfh, &info);
        MPI_Info_get_nkeys(info, &nkeys);

        printf("HINTS:\n");
        for (i = 0; i < nkeys; i++) {
            MPI_Info_get_nthkey(info, i, key);
            printf("%35s -> ", key);
            MPI_Info_get(info, key, 1024, value, &dummy_int);
            printf("%s\n", value);
        }
        MPI_Info_free(&info);
    }
    MPI_Barrier(MPI_COMM_WORLD);
}

static void fill_buffer(char *buffer, int bufsize, int rank, MPI_Offset offset)
{
    memset((void *) buffer, 0, bufsize);
    snprintf(buffer, bufsize, "Hello from %d at %lld\n", rank, offset);
}

static MPI_Offset get_offset(int rank, int num_objs, int obj_size, int which_obj)
{
    MPI_Offset offset;
    offset = (MPI_Offset) rank *num_objs * obj_size + which_obj * obj_size;
    return offset;
}

static void write_file(char *target, int rank, MPI_Info * info)
{
    MPI_File wfh;
    MPI_Request *request;
    MPI_Status *mpi_stat;
    int mpi_ret;
    int i;
    char **buffer;

    request = (MPI_Request *) malloc(NUM_OBJS * sizeof(MPI_Request));
    mpi_stat = (MPI_Status *) malloc(NUM_OBJS * sizeof(MPI_Status));
    buffer = (char **) malloc(NUM_OBJS * sizeof(char *));

    if (debug)
        printf("%d writing file %s\n", rank, target);

    if ((mpi_ret = MPI_File_open(MPI_COMM_WORLD, target,
                                 MPI_MODE_WRONLY | MPI_MODE_CREATE, *info, &wfh))
        != MPI_SUCCESS) {
        fatal_error(mpi_ret, NULL, "open for write");
    }

    /* nonblocking collective write */
    for (i = 0; i < NUM_OBJS; i++) {
        MPI_Offset offset = get_offset(rank, NUM_OBJS, OBJ_SIZE, i);
        buffer[i] = (char *) malloc(OBJ_SIZE);
        fill_buffer(buffer[i], OBJ_SIZE, rank, offset);
        if (debug)
            printf("%s", buffer[i]);
        if ((mpi_ret = MPI_File_iwrite_at_all(wfh, offset, buffer[i], OBJ_SIZE,
                                              MPI_CHAR, &request[i]))
            != MPI_SUCCESS) {
            fatal_error(mpi_ret, NULL, "write");
        }
    }

    if (debug)
        print_hints(rank, &wfh);

    MPI_Waitall(NUM_OBJS, request, mpi_stat);

    if ((mpi_ret = MPI_File_close(&wfh)) != MPI_SUCCESS) {
        fatal_error(mpi_ret, NULL, "close for write");
    }
    if (debug)
        printf("%d wrote file %s\n", rank, target);

    for (i = 0; i < NUM_OBJS; i++)
        free(buffer[i]);
    free(buffer);
    free(mpi_stat);
    free(request);
}

static void read_file(char *target, int rank, MPI_Info * info, int *corrupt_blocks)
{
    MPI_File rfh;
    MPI_Offset *offset;
    MPI_Request *request;
    MPI_Status *mpi_stat;
    int mpi_ret;
    int i;
    char **buffer;
    char **verify_buf = NULL;

    offset = (MPI_Offset *) malloc(NUM_OBJS * sizeof(MPI_Offset));
    request = (MPI_Request *) malloc(NUM_OBJS * sizeof(MPI_Request));
    mpi_stat = (MPI_Status *) malloc(NUM_OBJS * sizeof(MPI_Status));
    buffer = (char **) malloc(NUM_OBJS * sizeof(char *));
    verify_buf = (char **) malloc(NUM_OBJS * sizeof(char *));

    if (debug)
        printf("%d reading file %s\n", rank, target);

    if ((mpi_ret = MPI_File_open(MPI_COMM_WORLD, target, MPI_MODE_RDONLY,
                                 *info, &rfh)) != MPI_SUCCESS) {
        fatal_error(mpi_ret, NULL, "open for read");
    }

    /* nonblocking collective read */
    for (i = 0; i < NUM_OBJS; i++) {
        offset[i] = get_offset(rank, NUM_OBJS, OBJ_SIZE, i);
        buffer[i] = (char *) malloc(OBJ_SIZE);
        verify_buf[i] = (char *) malloc(OBJ_SIZE);
        fill_buffer(verify_buf[i], OBJ_SIZE, rank, offset[i]);
        if (debug)
            printf("Expecting %s", verify_buf[i]);
        if ((mpi_ret = MPI_File_iread_at_all(rfh, offset[i], buffer[i],
                                             OBJ_SIZE, MPI_CHAR, &request[i]))
            != MPI_SUCCESS) {
            fatal_error(mpi_ret, NULL, "read");
        }
    }

    MPI_Waitall(NUM_OBJS, request, mpi_stat);

    /* verification */
    for (i = 0; i < NUM_OBJS; i++) {
        if (memcmp(verify_buf[i], buffer[i], OBJ_SIZE) != 0) {
            (*corrupt_blocks)++;
            printf("Corruption at %lld\n", offset[i]);
            if (debug) {
                printf("\tExpecting %s\n" "\tRecieved  %s\n", verify_buf[i], buffer[i]);
            }
        }
    }

    if ((mpi_ret = MPI_File_close(&rfh)) != MPI_SUCCESS) {
        fatal_error(mpi_ret, NULL, "close for read");
    }

    for (i = 0; i < NUM_OBJS; i++) {
        free(verify_buf[i]);
        free(buffer[i]);
    }
    free(verify_buf);
    free(buffer);
    free(mpi_stat);
    free(request);
    free(offset);
}

static void set_hints(MPI_Info * info)
{
    MPI_Info_set(*info, "romio_cb_write", "enable");
    MPI_Info_set(*info, "romio_no_indep_rw", "1");
    MPI_Info_set(*info, "cb_nodes", "1");
    MPI_Info_set(*info, "cb_buffer_size", "4194304");
}

/*
void
set_hints(MPI_Info *info, char *hints) {
    char *delimiter = " ";
    char *hints_cp  = strdup(hints);
    char *key = strtok(hints_cp, delimiter);
    char *val;
    while (key) {
        val = strtok(NULL, delimiter);
        if (debug) printf("HINT: %s = %s\n", key, val);
        if (! val) {
            Usage(__LINE__);
        }
        MPI_Info_set(*info, key, val);
        key = strtok(NULL, delimiter);
    }
    free(hints_cp);
}
*/

int main(int argc, char *argv[])
{
    int nproc = 1, rank = 0;
    char *target = NULL;
    int c;
    MPI_Info info;
    int mpi_ret;
    int corrupt_blocks = 0;

    MTest_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if ((mpi_ret = MPI_Info_create(&info)) != MPI_SUCCESS) {
        if (rank == 0)
            fatal_error(mpi_ret, NULL, "MPI_info_create.\n");
    }

    prog = strdup(argv[0]);

    if (argc > 1) {
        while ((c = getopt(argc, argv, "df:h")) != EOF) {
            switch (c) {
                case 'd':
                    debug = 1;
                    break;
                case 'f':
                    target = strdup(optarg);
                    break;
                case 'h':
                    set_hints(&info);
                    break;
                default:
                    Usage(__LINE__);
            }
        }
        if (!target) {
            Usage(__LINE__);
        }
    } else {
        target = "testfile";
        set_hints(&info);
    }

    write_file(target, rank, &info);
    read_file(target, rank, &info, &corrupt_blocks);

    MPI_Info_free(&info);

    MTest_Finalize(corrupt_blocks);
    free(prog);
    return MTestReturnValue(corrupt_blocks);
}
