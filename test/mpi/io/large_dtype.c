/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * Copyright (C) 2024, Northwestern University
 *
 * This program tests large requests made by each MPI process.
 * Both buffer and fileview data types are noncontiguous.
 *
 * The local buffer datatype comprises NVARS arrays. Each array is 2D of size
 * len x len.  A gap of size GAP can be added at the end of each dimension to
 * make the data type noncontiguous.
 *
 * The fileview of each process comprises NVARS subarrays.  Each global array
 * is 2D of size (len * psize[0]) x (len * psize[1]). Each local array is of
 * size (len - GAP)  x (len - GAP).
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     /* getopt() */
#include <assert.h>

#include <mpi.h>

int Errors;

#define CHECK_MPI_ERROR(fname) \
    if (err != MPI_SUCCESS) { \
        int errorStringLen; \
        char errorString[MPI_MAX_ERROR_STRING]; \
        MPI_Error_string(err, errorString, &errorStringLen); \
        Errors++;\
        printf("Error at line %d when calling %s: %s\n",__LINE__,fname,errorString); \
    }

#define CHECK_MPIO_ERROR(fname) { \
    if (err != MPI_SUCCESS) { \
        int errorStringLen; \
        char errorString[MPI_MAX_ERROR_STRING]; \
        MPI_Error_string(err, errorString, &errorStringLen); \
        Errors++;\
        printf("Error at line %d when calling %s: %s\n",__LINE__,fname,errorString); \
    } \
    else if (verbose) { \
        if (rank == 0) \
            printf("---- pass LINE %d of calling %s\n", __LINE__, fname); \
        fflush(stdout); \
        MPI_Barrier(MPI_COMM_WORLD); \
    } \
}

#define LEN   2048
#define GAP   1
#define NVARS 1100

static void usage(char *argv0)
{
    char *help =
        "Usage: %s [-hvwr | -n num | -l num | -g num | file_name]\n"
        "       [-h] Print this help\n"
        "       [-v] verbose mode\n"
        "       [-w] performs write only (default: both write and read)\n"
        "       [-r] performs read  only (default: both write and read)\n"
        "       [-n num] number of global variables (default: %d)\n"
        "       [-l num] length of dimensions X and Y each local variable (default: %d)\n"
        "       [-g num] gap at the end of each dimension (default: %d)\n"
        "       [file_name] output file name\n";
    fprintf(stderr, help, argv0, NVARS, LEN, GAP);
}

/*----< main() >------------------------------------------------------------*/
int main(int argc, char **argv)
{
    char filename[512];
    size_t i, buf_len;
    int ret, err, rank, verbose, omode, nprocs, do_read, do_write;
    int nvars, len, gap, psize[2], gsize[2], count[2], start[2];
    char *buf;
    MPI_File fh;
    MPI_Datatype subType, filetype, buftype;
    MPI_Status status;
    MPI_Offset fsize;
    int *array_of_blocklengths;
    MPI_Aint *array_of_displacements;
    MPI_Datatype *array_of_types;
    MPI_Request req[2];

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    /* default values */
    len = LEN;
    gap = GAP;
    nvars = NVARS;
    do_write = 1;
    do_read = 1;
    verbose = 0;

    /* get command-line arguments */
    while ((ret = getopt(argc, argv, "hvwrn:l:g:")) != EOF)
        switch (ret) {
            case 'v':
                verbose = 1;
                break;
            case 'w':
                do_read = 0;
                break;
            case 'r':
                do_write = 0;
                break;
            case 'n':
                nvars = atoi(optarg);
                break;
            case 'l':
                len = atoi(optarg);
                break;
            case 'g':
                gap = atoi(optarg);
                break;
            case 'h':
            default:
                if (rank == 0)
                    usage(argv[0]);
                MPI_Finalize();
                return 1;
        }
    if (argv[optind] == NULL)
        sprintf(filename, "%s.out", argv[0]);
    else
        snprintf(filename, 256, "%s", argv[optind]);

    array_of_blocklengths = (int *) malloc(sizeof(int) * nvars);
    array_of_displacements = (MPI_Aint *) malloc(sizeof(MPI_Aint) * nvars);
    array_of_types = (MPI_Datatype *) malloc(sizeof(MPI_Datatype) * nvars);

    /* Creates a division of processors in a cartesian grid */
    psize[0] = psize[1] = 0;
    err = MPI_Dims_create(nprocs, 2, psize);
    CHECK_MPI_ERROR("MPI_Dims_create");

    /* each 2D variable is of size gsizes[0] x gsizes[1] bytes */
    gsize[0] = len * psize[0];
    gsize[1] = len * psize[1];

    /* set subarray offset and length */
    start[0] = len * (rank / psize[1]);
    start[1] = len * (rank % psize[1]);
    count[0] = len - gap;       /* -1 to create holes */
    count[1] = len - gap;

    fsize = (MPI_Offset) gsize[0] * gsize[1] * nvars - (len + 1);
    if (verbose) {
        buf_len = (size_t) nvars *(len - 1) * (len - 1);
        if (rank == 0) {
            printf("Output file name = %s\n", filename);
            printf("nprocs=%d nvars=%d len=%d\n", nprocs, nvars, len);
            printf("Expecting file size=%lld bytes (%.1f MB, %.1f GB)\n",
                   (long long) fsize * 2, (float) fsize * 2 / 1048576,
                   (float) fsize * 2 / 1073741824);
            printf("Each global variable is of size %d bytes (%.1f MB)\n", gsize[0] * gsize[1],
                   (float) gsize[0] * gsize[1] / 1048576);
            printf("Each process writes %zd bytes (%.1f MB, %.1f GB)\n", buf_len,
                   (float) buf_len / 1048576, (float) buf_len / 1073741824);
            printf("** For nonblocking I/O test, the amount is twice\n");
            printf("-------------------------------------------------------\n");
        }
        printf("rank %3d: gsize=%4d %4d start=%4d %4d count=%4d %4d\n", rank,
               gsize[0], gsize[1], start[0], start[1], count[0], count[1]);
    }

    /* create 2D subarray datatype for fileview */
    err = MPI_Type_create_subarray(2, gsize, count, start, MPI_ORDER_C, MPI_BYTE, &subType);
    CHECK_MPI_ERROR("MPI_Type_create_subarray");
    err = MPI_Type_commit(&subType);
    CHECK_MPI_ERROR("MPI_Type_commit");

    /* create a filetype by concatenating nvars subType */
    for (i = 0; i < nvars; i++) {
        array_of_blocklengths[i] = 1;
        array_of_displacements[i] = gsize[0] * gsize[1] * i;
        array_of_types[i] = subType;
    }
    err = MPI_Type_create_struct(nvars, array_of_blocklengths,
                                 array_of_displacements, array_of_types, &filetype);
    CHECK_MPI_ERROR("MPI_Type_create_struct");
    err = MPI_Type_commit(&filetype);
    CHECK_MPI_ERROR("MPI_Type_commit");
    err = MPI_Type_free(&subType);
    CHECK_MPI_ERROR("MPI_Type_free");

    /* Create local buffer datatype: each 2D variable is of size len x len */
    gsize[0] = len;
    gsize[1] = len;
    start[0] = 0;
    start[1] = 0;
    count[0] = len - gap;       /* -1 to create holes */
    count[1] = len - gap;

    err = MPI_Type_create_subarray(2, gsize, count, start, MPI_ORDER_C, MPI_BYTE, &subType);
    CHECK_MPI_ERROR("MPI_Type_create_subarray");
    err = MPI_Type_commit(&subType);
    CHECK_MPI_ERROR("MPI_Type_commit");

    /* concatenate nvars subType into a buftype */
    for (i = 0; i < nvars; i++) {
        array_of_blocklengths[i] = 1;
        array_of_displacements[i] = len * len * i;
        array_of_types[i] = subType;
    }

    /* create a buftype by concatenating nvars subTypes */
    err = MPI_Type_create_struct(nvars, array_of_blocklengths,
                                 array_of_displacements, array_of_types, &buftype);
    CHECK_MPI_ERROR("MPI_Type_create_struct");
    err = MPI_Type_commit(&buftype);
    CHECK_MPI_ERROR("MPI_Type_commit");
    err = MPI_Type_free(&subType);
    CHECK_MPI_ERROR("MPI_Type_free");

    free(array_of_blocklengths);
    free(array_of_displacements);
    free(array_of_types);

    /* allocate a local buffer */
    buf_len = (size_t) nvars *len * len;
    buf = (char *) malloc(buf_len);
    for (i = 0; i < buf_len; i++)
        buf[i] = (char) rank;

    /* open to create a file */
    omode = MPI_MODE_CREATE | MPI_MODE_RDWR;
    err = MPI_File_open(MPI_COMM_WORLD, filename, omode, MPI_INFO_NULL, &fh);
    CHECK_MPIO_ERROR("MPI_File_open");

    /* set the file view */
    err = MPI_File_set_view(fh, 0, MPI_BYTE, filetype, "native", MPI_INFO_NULL);
    CHECK_MPIO_ERROR("MPI_File_set_view");

    if (do_write) {
        err = MPI_File_seek(fh, 0, MPI_SEEK_SET);
        CHECK_MPIO_ERROR("MPI_File_seek");

        /* MPI collective write */
        err = MPI_File_write_all(fh, buf, 1, buftype, &status);
        CHECK_MPIO_ERROR("MPI_File_write_all");

        err = MPI_File_seek(fh, 0, MPI_SEEK_SET);
        CHECK_MPIO_ERROR("MPI_File_seek");

        /* MPI independent write */
        err = MPI_File_write(fh, buf, 1, buftype, &status);
        CHECK_MPIO_ERROR("MPI_File_write");

        /* MPI nonblocking collective write */
        char *buf2 = (char *) malloc(buf_len);
        for (i = 0; i < buf_len; i++)
            buf2[i] = (char) rank;

        err = MPI_File_seek(fh, 0, MPI_SEEK_SET);
        CHECK_MPIO_ERROR("MPI_File_seek");

        err = MPI_File_iwrite_all(fh, buf, 1, buftype, &req[0]);
        CHECK_MPIO_ERROR("MPI_File_iwrite_all 1");

        err = MPI_File_iwrite_all(fh, buf2, 1, buftype, &req[1]);
        CHECK_MPIO_ERROR("MPI_File_iwrite_all 2");

        err = MPI_Waitall(2, req, MPI_STATUSES_IGNORE);
        CHECK_MPIO_ERROR("MPI_Waitall");

        /* MPI nonblocking independent write */
        err = MPI_File_seek(fh, 0, MPI_SEEK_SET);
        CHECK_MPIO_ERROR("MPI_File_seek");

        err = MPI_File_iwrite(fh, buf, 1, buftype, &req[0]);
        CHECK_MPIO_ERROR("MPI_File_iwrite 1");

        err = MPI_File_iwrite(fh, buf2, 1, buftype, &req[1]);
        CHECK_MPIO_ERROR("MPI_File_iwrite 2");

        err = MPI_Waitall(2, req, MPI_STATUSES_IGNORE);
        CHECK_MPIO_ERROR("MPI_Waitall");

        free(buf2);
    }

    if (do_read) {
        err = MPI_File_seek(fh, 0, MPI_SEEK_SET);
        CHECK_MPIO_ERROR("MPI_File_seek");

        /* MPI collective read */
        err = MPI_File_read_all(fh, buf, 1, buftype, &status);
        CHECK_MPIO_ERROR("MPI_File_read_all");

        err = MPI_File_seek(fh, 0, MPI_SEEK_SET);
        CHECK_MPIO_ERROR("MPI_File_seek");

        /* MPI independent read */
        err = MPI_File_read(fh, buf, 1, buftype, &status);
        CHECK_MPIO_ERROR("MPI_File_read");

        /* MPI nonblocking collective read */
        char *buf2 = (char *) malloc(buf_len);
        for (i = 0; i < buf_len; i++)
            buf2[i] = (char) rank;

        err = MPI_File_seek(fh, 0, MPI_SEEK_SET);
        CHECK_MPIO_ERROR("MPI_File_seek");

        err = MPI_File_iread_all(fh, buf, 1, buftype, &req[0]);
        CHECK_MPIO_ERROR("MPI_File_iread_all 1");

        err = MPI_File_iread_all(fh, buf2, 1, buftype, &req[1]);
        CHECK_MPIO_ERROR("MPI_File_iread_all 2");

        err = MPI_Waitall(2, req, MPI_STATUSES_IGNORE);
        CHECK_MPIO_ERROR("MPI_Waitall");

        /* MPI nonblocking independent read */
        err = MPI_File_seek(fh, 0, MPI_SEEK_SET);
        CHECK_MPIO_ERROR("MPI_File_seek");

        err = MPI_File_iread(fh, buf, 1, buftype, &req[0]);
        CHECK_MPIO_ERROR("MPI_File_iread 1");

        err = MPI_File_iread(fh, buf2, 1, buftype, &req[1]);
        CHECK_MPIO_ERROR("MPI_File_iread 2");

        err = MPI_Waitall(2, req, MPI_STATUSES_IGNORE);
        CHECK_MPIO_ERROR("MPI_Waitall");

        free(buf2);
    }

    err = MPI_File_close(&fh);
    CHECK_MPIO_ERROR("MPI_File_close");

    free(buf);

    err = MPI_Type_free(&filetype);
    CHECK_MPI_ERROR("MPI_Type_free");
    err = MPI_Type_free(&buftype);
    CHECK_MPI_ERROR("MPI_Type_free");

    if (Errors > 0) {
        printf(" Found %d errors\n", Errors);
    } else {
        printf(" No Errors\n");
    }


    MPI_Finalize();
    return 0;
}
