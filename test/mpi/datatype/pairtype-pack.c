/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitestconf.h"
#ifdef HAVE_STRING_H
#include <string.h>
#endif

static int verbose = 0;

int short_int_pack_test(void);

/* helper functions */
int parse_args(int argc, char **argv);
static int pack_and_unpack(char *typebuf, int count, MPI_Datatype datatype, int typebufsz);

int main(int argc, char *argv[])
{
    int err, errs = 0;

    MPI_Init(&argc, &argv);     /* MPI-1.2 doesn't allow for MPI_Init(0,0) */
    parse_args(argc, argv);

    /* To improve reporting of problems about operations, we
     * change the error handler to errors return */
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    err = short_int_pack_test();
    errs += err;

    /* print message and exit */
    if (errs) {
        fprintf(stderr, "Found %d errors\n", errs);
    }
    else {
        printf(" No Errors\n");
    }
    MPI_Finalize();
    return 0;
}

int short_int_pack_test(void)
{
    int i, err, errs = 0;

    struct shortint {
        short a;
        int b;
    } sibuf[16];

    for (i = 0; i < 16; i++) {
        sibuf[i].a = (short) (i * 2);
        sibuf[i].b = i * 2 + 1;
    }

    err = pack_and_unpack((char *) sibuf, 16, MPI_SHORT_INT, sizeof(sibuf));
    if (err != 0) {
        if (verbose) {
            fprintf(stderr, "error packing/unpacking in short_int_pack_test()\n");
        }
        errs += err;
    }

    for (i = 0; i < 16; i++) {
        if (sibuf[i].a != (short) (i * 2)) {
            err++;
            if (verbose) {
                fprintf(stderr,
                        "buf[%d] has invalid short (%d); should be %d\n",
                        i, (int) sibuf[i].a, i * 2);
            }
        }
        if (sibuf[i].b != i * 2 + 1) {
            err++;
            if (verbose) {
                fprintf(stderr,
                        "buf[%d] has invalid int (%d); should be %d\n",
                        i, (int) sibuf[i].b, i * 2 + 1);
            }
        }
    }

    return errs;
}

/* pack_and_unpack()
 *
 * Perform packing and unpacking of a buffer for the purposes of checking
 * to see if we are processing a type correctly.  Zeros the buffer between
 * these two operations, so the data described by the type should be in
 * place upon return but all other regions of the buffer should be zero.
 *
 * Parameters:
 * typebuf - pointer to buffer described by datatype and count that
 *           will be packed and then unpacked into
 * count, datatype - description of typebuf
 * typebufsz - size of typebuf; used specifically to zero the buffer
 *             between the pack and unpack steps
 *
 */
static int pack_and_unpack(char *typebuf, int count, MPI_Datatype datatype, int typebufsz)
{
    char *packbuf;
    int err, errs = 0, pack_size, type_size, position;

    err = MPI_Type_size(datatype, &type_size);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose) {
            fprintf(stderr, "error in MPI_Type_size call; aborting after %d errors\n", errs);
        }
        return errs;
    }

    type_size *= count;

    err = MPI_Pack_size(count, datatype, MPI_COMM_SELF, &pack_size);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose) {
            fprintf(stderr, "error in MPI_Pack_size call; aborting after %d errors\n", errs);
        }
        return errs;
    }
    packbuf = (char *) malloc(pack_size);
    if (packbuf == NULL) {
        errs++;
        if (verbose) {
            fprintf(stderr, "error in malloc call; aborting after %d errors\n", errs);
        }
        return errs;
    }

    position = 0;
    err = MPI_Pack(typebuf, count, datatype, packbuf, type_size, &position, MPI_COMM_SELF);

    if (position != type_size) {
        errs++;
        if (verbose)
            fprintf(stderr, "position = %d; should be %d (pack)\n", position, type_size);
    }

    memset(typebuf, 0, typebufsz);
    position = 0;
    err = MPI_Unpack(packbuf, type_size, &position, typebuf, count, datatype, MPI_COMM_SELF);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose) {
            fprintf(stderr, "error in MPI_Unpack call; aborting after %d errors\n", errs);
        }
        return errs;
    }
    free(packbuf);

    if (position != type_size) {
        errs++;
        if (verbose)
            fprintf(stderr, "position = %d; should be %d (unpack)\n", position, type_size);
    }

    return errs;
}

int parse_args(int argc, char **argv)
{
    /*
     * int ret;
     *
     * while ((ret = getopt(argc, argv, "v")) >= 0)
     * {
     * switch (ret) {
     * case 'v':
     * verbose = 1;
     * break;
     * }
     * }
     */
    if (argc > 1 && strcmp(argv[1], "-v") == 0)
        verbose = 1;
    return 0;
}
