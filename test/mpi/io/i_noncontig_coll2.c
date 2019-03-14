/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpi.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mpitest.h"
#include "test_io.h"

/* tests noncontiguous reads/writes using nonblocking collective I/O */

/* this test is almost exactly like i_noncontig_coll.c with the following changes:
 *
 * . generalized file writing/reading to handle arbitrary number of processors
 * . provides the "cb_config_list" hint with several permutations of the
 *   avaliable processors.
 *   [ makes use of code copied from ROMIO's ADIO code to collect the names of
 *   the processors ]
 */

/* we are going to muck with this later to make it evenly divisible by however many compute nodes we have */
#define STARTING_SIZE 5000

int test_file(char *filename, int rank, int nprocs, char *cb_hosts, const char *msg, int verbose);

#define ADIOI_Free free
#define ADIOI_Malloc malloc
#define FPRINTF fprintf
/* I have no idea what the "D" stands for; it's how things are done in adio.h
 */
struct ADIO_cb_name_arrayD {
    int refct;
    int namect;
    char **names;
};
typedef struct ADIO_cb_name_arrayD *ADIO_cb_name_array;

void handle_error(int errcode, const char *str);
int cb_gather_name_array(MPI_Comm comm, ADIO_cb_name_array * arrayp);
void default_str(int rank, int len, ADIO_cb_name_array array, char *dest);
void reverse_str(int rank, int len, ADIO_cb_name_array array, char *dest);
void reverse_alternating_str(int rank, int len, ADIO_cb_name_array array, char *dest);
void simple_shuffle_str(int rank, int len, ADIO_cb_name_array array, char *dest);


void handle_error(int errcode, const char *str)
{
    char msg[MPI_MAX_ERROR_STRING];
    int resultlen;
    MPI_Error_string(errcode, msg, &resultlen);
    fprintf(stderr, "%s: %s\n", str, msg);
    MPI_Abort(MPI_COMM_WORLD, 1);
}


/* cb_gather_name_array() - gather a list of processor names from all processes
 *                          in a communicator and store them on rank 0.
 *
 * This is a collective call on the communicator(s) passed in.
 *
 * Obtains a rank-ordered list of processor names from the processes in
 * "dupcomm".
 *
 * Returns 0 on success, -1 on failure.
 *
 * NOTE: Needs some work to cleanly handle out of memory cases!
 */
int cb_gather_name_array(MPI_Comm comm, ADIO_cb_name_array * arrayp)
{
    /* this is copied from ROMIO, but since this test is for correctness,
     * not performance, note that we have removed the parts where ROMIO
     * uses a keyval to cache the name array.  We'll just rebuild it if we
     * need to */

    char my_procname[MPI_MAX_PROCESSOR_NAME], **procname = 0;
    int *procname_len = NULL, my_procname_len, *disp = NULL, i;
    int commsize, commrank;
    ADIO_cb_name_array array = NULL;

    MPI_Comm_size(comm, &commsize);
    MPI_Comm_rank(comm, &commrank);

    MPI_Get_processor_name(my_procname, &my_procname_len);

    /* allocate space for everything */
    array = (ADIO_cb_name_array) malloc(sizeof(*array));
    if (array == NULL) {
        return -1;
    }
    array->refct = 1;

    if (commrank == 0) {
        /* process 0 keeps the real list */
        array->namect = commsize;

        array->names = (char **) ADIOI_Malloc(sizeof(char *) * commsize);
        if (array->names == NULL) {
            return -1;
        }
        procname = array->names;        /* simpler to read */

        procname_len = (int *) ADIOI_Malloc(commsize * sizeof(int));
        if (procname_len == NULL) {
            return -1;
        }
    } else {
        /* everyone else just keeps an empty list as a placeholder */
        array->namect = 0;
        array->names = NULL;
    }
    /* gather lengths first */
    MPI_Gather(&my_procname_len, 1, MPI_INT, procname_len, 1, MPI_INT, 0, comm);

    if (commrank == 0) {
#ifdef CB_CONFIG_LIST_DEBUG
        for (i = 0; i < commsize; i++) {
            FPRINTF(stderr, "len[%d] = %d\n", i, procname_len[i]);
        }
#endif

        for (i = 0; i < commsize; i++) {
            /* add one to the lengths because we need to count the
             * terminator, and we are going to use this list of lengths
             * again in the gatherv.
             */
            procname_len[i]++;
            procname[i] = malloc(procname_len[i]);
            if (procname[i] == NULL) {
                return -1;
            }
        }

        /* create our list of displacements for the gatherv.  we're going
         * to do everything relative to the start of the region allocated
         * for procname[0]
         *
         * I suppose it is theoretically possible that the distance between
         * malloc'd regions could be more than will fit in an int.  We don't
         * cover that case.
         */
        disp = malloc(commsize * sizeof(int));
        disp[0] = 0;
        for (i = 1; i < commsize; i++) {
            disp[i] = (int) (procname[i] - procname[0]);
        }
    }

    /* now gather strings */
    if (commrank == 0) {
        MPI_Gatherv(my_procname, my_procname_len + 1, MPI_CHAR,
                    procname[0], procname_len, disp, MPI_CHAR, 0, comm);
    } else {
        /* if we didn't do this, we would need to allocate procname[]
         * on all processes...which seems a little silly.
         */
        MPI_Gatherv(my_procname, my_procname_len + 1, MPI_CHAR,
                    NULL, NULL, NULL, MPI_CHAR, 0, comm);
    }

    if (commrank == 0) {
        /* no longer need the displacements or lengths */
        free(disp);
        free(procname_len);

#ifdef CB_CONFIG_LIST_DEBUG
        for (i = 0; i < commsize; i++) {
            fprintf(stderr, "name[%d] = %s\n", i, procname[i]);
        }
#endif
    }

    *arrayp = array;
    return 0;
}

void default_str(int rank, int len, ADIO_cb_name_array array, char *dest)
{
    char *ptr;
    int i, p;
    if (!rank) {
        ptr = dest;
        for (i = 0; i < array->namect; i++) {
            p = snprintf(ptr, len, "%s,", array->names[i]);
            ptr += p;
        }
        /* chop off that last comma */
        dest[strlen(dest) - 1] = '\0';
    }
    MPI_Bcast(dest, len, MPI_CHAR, 0, MPI_COMM_WORLD);
}

void reverse_str(int rank, int len, ADIO_cb_name_array array, char *dest)
{
    char *ptr;
    int i, p;
    if (!rank) {
        ptr = dest;
        for (i = (array->namect - 1); i >= 0; i--) {
            p = snprintf(ptr, len, "%s,", array->names[i]);
            ptr += p;
        }
        dest[strlen(dest) - 1] = '\0';
    }
    MPI_Bcast(dest, len, MPI_CHAR, 0, MPI_COMM_WORLD);
}

void reverse_alternating_str(int rank, int len, ADIO_cb_name_array array, char *dest)
{
    char *ptr;
    int i, p;
    if (!rank) {
        ptr = dest;
        /* evens */
        for (i = (array->namect - 1); i >= 0; i -= 2) {
            p = snprintf(ptr, len, "%s,", array->names[i]);
            ptr += p;
        }
        /* odds */
        for (i = (array->namect - 2); i > 0; i -= 2) {
            p = snprintf(ptr, len, "%s,", array->names[i]);
            ptr += p;
        }
        dest[strlen(dest) - 1] = '\0';
    }
    MPI_Bcast(dest, len, MPI_CHAR, 0, MPI_COMM_WORLD);
}

void simple_shuffle_str(int rank, int len, ADIO_cb_name_array array, char *dest)
{
    char *ptr;
    int i, p;
    if (!rank) {
        ptr = dest;
        for (i = (array->namect / 2); i < array->namect; i++) {
            p = snprintf(ptr, len, "%s,", array->names[i]);
            ptr += p;
        }
        for (i = 0; i < (array->namect / 2); i++) {
            p = snprintf(ptr, len, "%s,", array->names[i]);
            ptr += p;
        }
        dest[strlen(dest) - 1] = '\0';
    }
    MPI_Bcast(dest, len, MPI_CHAR, 0, MPI_COMM_WORLD);
}

int main(int argc, char **argv)
{
    int i, rank, nprocs, errs = 0, sum_errs = 0, verbose = 0;
    char *cb_config_string;
    int cb_config_len;
    ADIO_cb_name_array array;
    INIT_FILENAME;

    MTest_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    GET_TEST_FILENAME;

    /* want to hint the cb_config_list, but do so in a non-sequential way */
    cb_gather_name_array(MPI_COMM_WORLD, &array);

    /* sanity check */
    if (!rank) {
        if (array->namect < 2) {
            fprintf(stderr, "Run this test on two or more hosts\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }
    /* get space for the permuted cb_config_string */
    if (!rank) {
        cb_config_len = 0;
        for (i = 0; i < array->namect; i++) {
            /* +1: space for either a , or \0 if last */
            cb_config_len += strlen(array->names[i]) + 1;
        }
        ++cb_config_len;
    }
    MPI_Bcast(&cb_config_len, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if ((cb_config_string = malloc(cb_config_len)) == NULL) {
        perror("malloc");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /* first, no hinting */
    errs += test_file(filename, rank, nprocs, NULL, "collective w/o hinting", verbose);

    /* hint, but no change in order */
    default_str(rank, cb_config_len, array, cb_config_string);
    errs += test_file(filename, rank, nprocs, cb_config_string,
                      "collective w/ hinting: default order", verbose);

    /*  reverse order */
    reverse_str(rank, cb_config_len, array, cb_config_string);
    errs += test_file(filename, rank, nprocs, cb_config_string,
                      "collective w/ hinting: reverse order", verbose);

    /* reverse, every other */
    reverse_alternating_str(rank, cb_config_len, array, cb_config_string);
    errs += test_file(filename, rank, nprocs, cb_config_string,
                      "collective w/ hinting: permutation1", verbose);

    /* second half, first half */
    simple_shuffle_str(rank, cb_config_len, array, cb_config_string);
    errs += test_file(filename, rank, nprocs, cb_config_string,
                      "collective w/ hinting: permutation2", verbose);

    free(cb_config_string);
    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}

#define SEEDER(x,y,z) ((x)*1000000 + (y) + (x)*(z))

int test_file(char *filename, int rank, int nprocs, char *cb_hosts, const char *msg, int verbose)
{
    MPI_Datatype typevec, newtype, t[3];
    int *buf, i, b[3], errcode, errors = 0;
    MPI_File fh;
    MPI_Aint d[3];
    MPI_Request request;
    MPI_Status status;
    int SIZE = (STARTING_SIZE / nprocs) * nprocs;
    MPI_Info info;

    if (rank == 0 && verbose)
        fprintf(stderr, "%s\n", msg);

    buf = (int *) malloc(SIZE * sizeof(int));
    if (buf == NULL) {
        perror("test_file");
        MPI_Abort(MPI_COMM_WORLD, -1);
    }


    if (cb_hosts != NULL) {
        MPI_Info_create(&info);
        MPI_Info_set(info, "cb_config_list", cb_hosts);
    } else {
        info = MPI_INFO_NULL;
    }

    MPI_Type_vector(SIZE / nprocs, 1, nprocs, MPI_INT, &typevec);

    b[0] = b[1] = b[2] = 1;
    d[0] = 0;
    d[1] = rank * sizeof(int);
    d[2] = SIZE * sizeof(int);
    t[0] = MPI_LB;
    t[1] = typevec;
    t[2] = MPI_UB;

    MPI_Type_struct(3, b, d, t, &newtype);
    MPI_Type_commit(&newtype);
    MPI_Type_free(&typevec);

    if (!rank) {
        if (verbose)
            fprintf(stderr, "\ntesting noncontiguous in memory, noncontiguous "
                    "in file using collective I/O\n");
        MPI_File_delete(filename, info);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    errcode = MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_CREATE | MPI_MODE_RDWR, info, &fh);
    if (errcode != MPI_SUCCESS) {
        handle_error(errcode, "MPI_File_open");
    }

    errcode = MPI_File_set_view(fh, 0, MPI_INT, newtype, "native", info);
    if (errcode != MPI_SUCCESS) {
        handle_error(errcode, "MPI_File_set_view");
    }

    for (i = 0; i < SIZE; i++)
        buf[i] = SEEDER(rank, i, SIZE);
    errcode = MPI_File_iwrite_all(fh, buf, 1, newtype, &request);
    if (errcode != MPI_SUCCESS) {
        handle_error(errcode, "nc mem - nc file: MPI_File_iwrite_all");
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Wait(&request, &status);

    for (i = 0; i < SIZE; i++)
        buf[i] = -1;

    errcode = MPI_File_iread_at_all(fh, 0, buf, 1, newtype, &request);
    if (errcode != MPI_SUCCESS) {
        handle_error(errcode, "nc mem - nc file: MPI_File_iread_at_all");
    }
    MPI_Wait(&request, &status);

    /* the verification for N compute nodes is tricky. Say we have 3
     * processors.
     * process 0 sees: 0 -1 -1 3 -1 -1 ...
     * process 1 sees: -1 34 -1 -1 37 -1 ...
     * process 2 sees: -1 -1 68 -1 -1 71 ... */

    /* verify those leading -1s exist if they should */
    for (i = 0; i < rank; i++) {
        if (buf[i] != -1) {
            if (verbose)
                fprintf(stderr, "Process %d: buf is %d, should be -1\n", rank, buf[i]);
            errors++;
        }
    }
    /* now the modulo games are hairy.  processor 0 sees real data in the 0th,
     * 3rd, 6th... elements of the buffer (assuming nprocs==3).  proc 1 sees
     * the data in 1st, 4th, 7th..., and proc 2 sees it in 2nd, 5th, 8th */

    for (/* 'i' set in above loop */ ; i < SIZE; i++) {
        if (((i - rank) % nprocs) && buf[i] != -1) {
            if (verbose)
                fprintf(stderr, "Process %d: buf %d is %d, should be -1\n", rank, i, buf[i]);
            errors++;
        }
        if (!((i - rank) % nprocs) && buf[i] != SEEDER(rank, i, SIZE)) {
            if (verbose)
                fprintf(stderr, "Process %d: buf %d is %d, should be %d\n",
                        rank, i, buf[i], SEEDER(rank, i, SIZE));
            errors++;
        }
    }
    MPI_File_close(&fh);

    MPI_Barrier(MPI_COMM_WORLD);

    if (!rank) {
        if (verbose)
            fprintf(stderr, "\ntesting noncontiguous in memory, contiguous in "
                    "file using collective I/O\n");
        MPI_File_delete(filename, info);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_CREATE | MPI_MODE_RDWR, info, &fh);

    for (i = 0; i < SIZE; i++)
        buf[i] = SEEDER(rank, i, SIZE);
    errcode = MPI_File_iwrite_at_all(fh, rank * (SIZE / nprocs) * sizeof(int),
                                     buf, 1, newtype, &request);
    if (errcode != MPI_SUCCESS)
        handle_error(errcode, "nc mem - c file: MPI_File_iwrite_at_all");

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Wait(&request, &status);

    for (i = 0; i < SIZE; i++)
        buf[i] = -1;

    errcode = MPI_File_iread_at_all(fh, rank * (SIZE / nprocs) * sizeof(int),
                                    buf, 1, newtype, &request);
    if (errcode != MPI_SUCCESS)
        handle_error(errcode, "nc mem - c file: MPI_File_iread_at_all");
    MPI_Wait(&request, &status);

    /* just like as above */
    for (i = 0; i < rank; i++) {
        if (buf[i] != -1) {
            if (verbose)
                fprintf(stderr, "Process %d: buf is %d, should be -1\n", rank, buf[i]);
            errors++;
        }
    }
    for (/* i set in above loop */ ; i < SIZE; i++) {
        if (((i - rank) % nprocs) && buf[i] != -1) {
            if (verbose)
                fprintf(stderr, "Process %d: buf %d is %d, should be -1\n", rank, i, buf[i]);
            errors++;
        }
        if (!((i - rank) % nprocs) && buf[i] != SEEDER(rank, i, SIZE)) {
            if (verbose)
                fprintf(stderr, "Process %d: buf %d is %d, should be %d\n",
                        rank, i, buf[i], SEEDER(rank, i, SIZE));
            errors++;
        }
    }

    MPI_File_close(&fh);

    MPI_Barrier(MPI_COMM_WORLD);

    if (!rank) {
        if (verbose)
            fprintf(stderr, "\ntesting contiguous in memory, noncontiguous in "
                    "file using collective I/O\n");
        MPI_File_delete(filename, info);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_CREATE | MPI_MODE_RDWR, info, &fh);

    errcode = MPI_File_set_view(fh, 0, MPI_INT, newtype, "native", info);
    if (errcode != MPI_SUCCESS) {
        handle_error(errcode, "MPI_File_set_view");
    }

    for (i = 0; i < SIZE; i++)
        buf[i] = SEEDER(rank, i, SIZE);
    errcode = MPI_File_iwrite_all(fh, buf, SIZE, MPI_INT, &request);
    if (errcode != MPI_SUCCESS)
        handle_error(errcode, "c mem - nc file: MPI_File_iwrite_all");

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Wait(&request, &status);

    for (i = 0; i < SIZE; i++)
        buf[i] = -1;

    errcode = MPI_File_iread_at_all(fh, 0, buf, SIZE, MPI_INT, &request);
    if (errcode != MPI_SUCCESS)
        handle_error(errcode, "c mem - nc file: MPI_File_iread_at_all");
    MPI_Wait(&request, &status);

    /* same crazy checking */
    for (i = 0; i < SIZE; i++) {
        if (buf[i] != SEEDER(rank, i, SIZE)) {
            if (verbose)
                fprintf(stderr, "Process %d: buf %d is %d, should be %d\n",
                        rank, i, buf[i], SEEDER(rank, i, SIZE));
            errors++;
        }
    }

    MPI_File_close(&fh);

    MPI_Type_free(&newtype);
    free(buf);
    if (info != MPI_INFO_NULL)
        MPI_Info_free(&info);
    return errors;
}
