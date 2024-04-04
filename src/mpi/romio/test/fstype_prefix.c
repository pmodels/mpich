/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>     /* strcmp() */
#include <romioconf.h>

#include <mpi.h>

/* set a file system type prefix known to ROMIO which is enabled at configure */
static const char *enabled_prefix =
#ifdef ROMIO_UFS
    "ufs:";
#elif defined(ROMIO_NFS)
    "nfs:";
#elif defined(ROMIO_XFS)
    "xfs:";
#elif defined(ROMIO_PVFS2)
    "pvfs2:";
#elif defined(ROMIO_GPFS)
    "gpfs:";
#elif defined(ROMIO_PANFS)
    "panfs:";
#elif defined(ROMIO_LUSTRE)
    "lustre:";
#elif defined(ROMIO_DAOS)
    "daos:";
#elif defined(ROMIO_TESTFS)
    "testfs:";
#elif defined(ROMIO_IME)
    "ime:";
#elif defined(ROMIO_QUOBYTEFS)
    "quobyte:";
#else
    ;
#error "ROMIO: no file system is enabled"
#endif

/* set a file system type prefix known to ROMIO but not enabled at configure */
static const char *disabled_prefix =
#ifndef ROMIO_UFS
    "ufs:";
#elif !defined(ROMIO_NFS)
    "nfs:";
#elif !defined(ROMIO_XFS)
    "xfs:";
#elif !defined(ROMIO_PVFS2)
    "pvfs2:";
#elif !defined(ROMIO_GPFS)
    "gpfs:";
#elif !defined(ROMIO_PANFS)
    "panfs:";
#elif !defined(ROMIO_LUSTRE)
    "lustre:";
#elif !defined(ROMIO_DAOS)
    "daos:";
#elif !defined(ROMIO_TESTFS)
    "testfs:";
#elif !defined(ROMIO_IME)
    "ime:";
#elif !defined(ROMIO_QUOBYTEFS)
    "quobyte:";
#endif

static
void err_expected(int err, int exp_err)
{
    int rank, errorclass;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Error_class(err, &errorclass);
    if (exp_err != errorclass) {
        fprintf(stderr, "rank %d: MPI error class (%d) is not expected (%d)\n", rank, errorclass,
                exp_err);

        MPI_Abort(MPI_COMM_WORLD, -1);
        exit(1);
    }
}

static
void err_handler(int err, const char *err_msg)
{
    int rank, errorStringLen;
    char errorString[MPI_MAX_ERROR_STRING];

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Error_string(err, errorString, &errorStringLen);
    if (err_msg == NULL)
        fprintf(stderr, "rank %d: MPI error : %s\n", rank, errorString);
    else
        fprintf(stderr, "rank %d: MPI error (%s) : %s\n", rank, err_msg, errorString);
    MPI_Abort(MPI_COMM_WORLD, -1);
    exit(1);
}

static int check_file_exist(const char *out_fname)
{
    int err, rank;
    MPI_File fh;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) {
        err = MPI_File_open(MPI_COMM_SELF, out_fname, MPI_MODE_RDONLY, MPI_INFO_NULL, &fh);
        if (err != MPI_SUCCESS)
            err = -1;
        else
            MPI_File_close(&fh);
    }
    MPI_Bcast(&err, 1, MPI_INT, 0, MPI_COMM_WORLD);
    return err;
}

static int delete_file(const char *out_fname)
{
    int err, rank;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) {
        err = MPI_File_delete(out_fname, MPI_INFO_NULL);
        if (err != MPI_SUCCESS)
            err = -1;
    }
    MPI_Bcast(&err, 1, MPI_INT, 0, MPI_COMM_WORLD);
    return err;
}

int main(int argc, char **argv)
{
    int i, err = 0, verbose = 0, rank, len;
    char *filename, out_fname[512];
    MPI_File fh;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

/* process 0 takes the file name as a command-line argument and
   broadcasts it to other processes */
    if (!rank) {
        i = 1;
        while ((i < argc) && strcmp("-fname", *argv)) {
            i++;
            argv++;
        }
        if (i >= argc) {
            fprintf(stderr, "\n*#  Usage: %s -fname filename\n\n", argv[0]);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        argv++;
        len = strlen(*argv);
        filename = (char *) malloc(len + 10);
        strcpy(filename, *argv);
        MPI_Bcast(&len, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(filename, len + 10, MPI_CHAR, 0, MPI_COMM_WORLD);
    } else {
        MPI_Bcast(&len, 1, MPI_INT, 0, MPI_COMM_WORLD);
        filename = (char *) malloc(len + 10);
        MPI_Bcast(filename, len + 10, MPI_CHAR, 0, MPI_COMM_WORLD);
    }

    /* test a file system type prefix unknown to ROMIO */
    sprintf(out_fname, "nosuch_fstype:%s.out", filename);
    if (verbose && rank == 0)
        fprintf(stdout, "Testing file name prefix (unknown to ROMIO): %s", out_fname);
    err = MPI_File_open(MPI_COMM_WORLD, out_fname, MPI_MODE_CREATE | MPI_MODE_RDWR,
                        MPI_INFO_NULL, &fh);
    if (err != MPI_SUCCESS)
        err_handler(err, "MPI_File_open()");
    err = MPI_File_close(&fh);
    if (err != MPI_SUCCESS)
        err_handler(err, "MPI_File_close()");

    MPI_Barrier(MPI_COMM_WORLD);

    err = check_file_exist(out_fname);
    if (err != 0)
        goto err_out;

    err = delete_file(out_fname);
    if (err != 0)
        goto err_out;
    if (verbose && rank == 0)
        fprintf(stdout, " ---- pass\n");

    MPI_Barrier(MPI_COMM_WORLD);

    /* test a file system type prefix known to ROMIO and enabled at configure */
    sprintf(out_fname, "%s%s.out", enabled_prefix, filename);
    if (verbose && rank == 0)
        fprintf(stdout, "Testing file name prefix (known and enabled): %s", enabled_prefix);
    err = MPI_File_open(MPI_COMM_WORLD, out_fname, MPI_MODE_CREATE | MPI_MODE_RDWR,
                        MPI_INFO_NULL, &fh);
    if (err != MPI_SUCCESS)
        err_handler(err, "MPI_File_open()");
    err = MPI_File_close(&fh);
    if (err != MPI_SUCCESS)
        err_handler(err, "MPI_File_close()");

    MPI_Barrier(MPI_COMM_WORLD);

    /* strip the known prefix */
    sprintf(out_fname, "%s.out", filename);
    err = check_file_exist(out_fname);
    if (err != 0)
        goto err_out;

    err = delete_file(out_fname);
    if (err != 0)
        goto err_out;
    if (verbose && rank == 0)
        fprintf(stdout, " ---- pass\n");

    MPI_Barrier(MPI_COMM_WORLD);

    /* set a known file system type prefix to ROMIO in environment variable ROMIO_FSTYPE_FORCE */
    setenv("ROMIO_FSTYPE_FORCE", enabled_prefix, 1);
    sprintf(out_fname, "%s.out", filename);
    if (verbose && rank == 0)
        fprintf(stdout, "Testing ROMIO_FSTYPE_FORCE prefix (known and enabled): %s",
                enabled_prefix);
    err = MPI_File_open(MPI_COMM_WORLD, out_fname, MPI_MODE_CREATE | MPI_MODE_RDWR, MPI_INFO_NULL,
                        &fh);
    if (err != MPI_SUCCESS)
        err_handler(err, "MPI_File_open()");
    err = MPI_File_close(&fh);
    if (err != MPI_SUCCESS)
        err_handler(err, "MPI_File_close()");

    MPI_Barrier(MPI_COMM_WORLD);

    err = check_file_exist(out_fname);
    if (err != 0)
        goto err_out;

    err = delete_file(out_fname);
    if (err != 0)
        goto err_out;
    if (verbose && rank == 0)
        fprintf(stdout, " ---- pass\n");

    MPI_Barrier(MPI_COMM_WORLD);

    /* test a file system type prefix known to ROMIO but disabled at configure */
    sprintf(out_fname, "%s%s.out", disabled_prefix, filename);
    if (verbose && rank == 0)
        fprintf(stdout, "Testing file name prefix (known but disabled): %s", disabled_prefix);
    err = MPI_File_open(MPI_COMM_WORLD, out_fname, MPI_MODE_CREATE | MPI_MODE_RDWR,
                        MPI_INFO_NULL, &fh);
    err_expected(err, MPI_ERR_IO);
    if (verbose && rank == 0)
        fprintf(stdout, " ---- pass\n");

    MPI_Barrier(MPI_COMM_WORLD);

    /* set a known file system type prefix to ROMIO in environment variable ROMIO_FSTYPE_FORCE */
    setenv("ROMIO_FSTYPE_FORCE", disabled_prefix, 1);
    sprintf(out_fname, "%s.out", filename);
    if (verbose && rank == 0)
        fprintf(stdout, "Testing ROMIO_FSTYPE_FORCE prefix (known but disabled): %s",
                disabled_prefix);
    err = MPI_File_open(MPI_COMM_WORLD, out_fname, MPI_MODE_CREATE | MPI_MODE_RDWR, MPI_INFO_NULL,
                        &fh);
    err_expected(err, MPI_ERR_IO);
    if (verbose && rank == 0)
        fprintf(stdout, " ---- pass\n");

    err = 0;

  err_out:
    if (rank == 0 && err == 0)
        fprintf(stdout, " No Errors\n");

    free(filename);
    MPI_Finalize();
    return err;
}
