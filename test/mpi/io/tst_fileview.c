#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#define CHECK_ERROR(err, nerrs)                                         \
    do {                                                                \
        if ((err) != MPI_SUCCESS) {                                     \
            int errorStringLen;                                         \
            char errorString[MPI_MAX_ERROR_STRING];                     \
            MPI_Error_string((err), errorString, &errorStringLen);      \
            printf("Error at line %d: %s\n",__LINE__, errorString);     \
            (nerrs)++;                                                  \
        }                                                               \
    } while (0)

#define CHECK_EXPECTED_ERROR(exp, err, nerrs)                           \
    do {                                                                \
        int  errorStringLen, errorclass;                                \
        MPI_Error_class((err), &errorclass);                            \
        if (errorclass != (exp)) {                                      \
            char errorString[MPI_MAX_ERROR_STRING+1];                   \
            MPI_Error_string((err), errorString, &errorStringLen);      \
            printf("Error at line %d: expect %s but got %s\n",          \
                   __LINE__,#exp, errorString);                         \
            (nerrs)++;                                                  \
        }                                                               \
    } while (0)

int main(int argc, char *argv[])
{
    char *filename;
    int i, len, nprocs, amode, err, nerrs = 0;
    int blen[2], disp[2];
    MPI_Datatype etype, filetype;
    MPI_File fh;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    if (nprocs != 1) {
        fprintf(stderr, "Run this program on 1 process\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    i = 1;
    while ((i < argc) && strcmp("-fname", *argv)) {
        i++;
        argv++;
    }
    if (i >= argc) {
        len = 8;
        filename = (char *) malloc(len + 10);
        strcpy(filename, "testfile");
    } else {
        argv++;
        len = (int) strlen(*argv);
        filename = (char *) malloc(len + 1);
        strcpy(filename, *argv);
    }

    MPI_File_delete(filename, MPI_INFO_NULL);

    amode = MPI_MODE_RDWR | MPI_MODE_CREATE;
    err = MPI_File_open(MPI_COMM_WORLD, filename, amode, MPI_INFO_NULL, &fh);
    CHECK_ERROR(err, nerrs);

    /* create etype with negative disp */
    disp[0] = -2;
    disp[1] = 2;
    blen[0] = 1;
    blen[1] = 1;
    err = MPI_Type_indexed(2, blen, disp, MPI_INT, &etype);
    CHECK_ERROR(err, nerrs);

    err = MPI_Type_commit(&etype);
    CHECK_ERROR(err, nerrs);

    err = MPI_File_set_view(fh, 0, etype, etype, "native", MPI_INFO_NULL);
    CHECK_EXPECTED_ERROR(MPI_ERR_IO, err, nerrs);
    err = MPI_Type_free(&etype);
    CHECK_ERROR(err, nerrs);

    /* create etype with decreasing disp */
    disp[0] = 3;
    blen[0] = 1;
    disp[1] = 0;
    blen[1] = 1;
    err = MPI_Type_indexed(2, blen, disp, MPI_INT, &etype);
    CHECK_ERROR(err, nerrs);

    err = MPI_Type_commit(&etype);
    CHECK_ERROR(err, nerrs);

    err = MPI_File_set_view(fh, 0, etype, etype, "native", MPI_INFO_NULL);
    CHECK_EXPECTED_ERROR(MPI_ERR_IO, err, nerrs);

    err = MPI_Type_free(&etype);
    CHECK_ERROR(err, nerrs);

    /* create etype with overlaps */
    disp[0] = 0;
    blen[0] = 3;
    disp[1] = 1;
    blen[1] = 1;
    err = MPI_Type_indexed(2, blen, disp, MPI_INT, &etype);
    CHECK_ERROR(err, nerrs);

    err = MPI_Type_commit(&etype);
    CHECK_ERROR(err, nerrs);

    err = MPI_File_set_view(fh, 0, etype, etype, "native", MPI_INFO_NULL);
    CHECK_EXPECTED_ERROR(MPI_ERR_IO, err, nerrs);

    err = MPI_Type_free(&etype);
    CHECK_ERROR(err, nerrs);

    /* create filetype with negative disp */
    disp[0] = -2;
    disp[1] = 2;
    blen[0] = 1;
    blen[1] = 1;
    err = MPI_Type_indexed(2, blen, disp, MPI_INT, &filetype);
    CHECK_ERROR(err, nerrs);

    err = MPI_Type_commit(&filetype);
    CHECK_ERROR(err, nerrs);

    err = MPI_File_set_view(fh, 0, MPI_INT, filetype, "native", MPI_INFO_NULL);
    CHECK_EXPECTED_ERROR(MPI_ERR_IO, err, nerrs);

    err = MPI_Type_free(&filetype);
    CHECK_ERROR(err, nerrs);

    /* create filetype with decreasing disp */
    disp[0] = 3;
    blen[0] = 1;
    disp[1] = 0;
    blen[1] = 1;
    err = MPI_Type_indexed(2, blen, disp, MPI_INT, &filetype);
    CHECK_ERROR(err, nerrs);

    err = MPI_Type_commit(&filetype);
    CHECK_ERROR(err, nerrs);

    err = MPI_File_set_view(fh, 0, MPI_INT, filetype, "native", MPI_INFO_NULL);
    CHECK_EXPECTED_ERROR(MPI_ERR_IO, err, nerrs);

    err = MPI_Type_free(&filetype);
    CHECK_ERROR(err, nerrs);

    /* create filetype with overlaps */
    disp[0] = 0;
    blen[0] = 3;
    disp[1] = 1;
    blen[1] = 1;
    err = MPI_Type_indexed(2, blen, disp, MPI_INT, &filetype);
    CHECK_ERROR(err, nerrs);

    err = MPI_Type_commit(&filetype);
    CHECK_ERROR(err, nerrs);

    err = MPI_File_set_view(fh, 0, MPI_INT, filetype, "native", MPI_INFO_NULL);
    CHECK_EXPECTED_ERROR(MPI_ERR_IO, err, nerrs);

    err = MPI_Type_free(&filetype);
    CHECK_ERROR(err, nerrs);

    err = MPI_File_close(&fh);
    CHECK_ERROR(err, nerrs);

    /* open the file for read only */
    amode = MPI_MODE_RDONLY;
    err = MPI_File_open(MPI_COMM_WORLD, filename, amode, MPI_INFO_NULL, &fh);
    CHECK_ERROR(err, nerrs);

    /* create etype with negative disp */
    disp[0] = -2;
    disp[1] = 2;
    blen[0] = 1;
    blen[1] = 1;
    err = MPI_Type_indexed(2, blen, disp, MPI_INT, &etype);
    CHECK_ERROR(err, nerrs);

    err = MPI_Type_commit(&etype);
    CHECK_ERROR(err, nerrs);

    err = MPI_File_set_view(fh, 0, etype, etype, "native", MPI_INFO_NULL);
    CHECK_EXPECTED_ERROR(MPI_ERR_IO, err, nerrs);

    err = MPI_Type_free(&etype);
    CHECK_ERROR(err, nerrs);

    /* create etype with decreasing disp */
    disp[0] = 3;
    blen[0] = 1;
    disp[1] = 0;
    blen[1] = 1;
    err = MPI_Type_indexed(2, blen, disp, MPI_INT, &etype);
    CHECK_ERROR(err, nerrs);

    err = MPI_Type_commit(&etype);
    CHECK_ERROR(err, nerrs);

    err = MPI_File_set_view(fh, 0, etype, etype, "native", MPI_INFO_NULL);
    CHECK_EXPECTED_ERROR(MPI_ERR_IO, err, nerrs);

    err = MPI_Type_free(&etype);
    CHECK_ERROR(err, nerrs);

    /* create etype with overlaps (should be OK for read-only) */
    disp[0] = 0;
    blen[0] = 3;
    disp[1] = 1;
    blen[1] = 1;
    err = MPI_Type_indexed(2, blen, disp, MPI_INT, &etype);
    CHECK_ERROR(err, nerrs);

    err = MPI_Type_commit(&etype);
    CHECK_ERROR(err, nerrs);

    err = MPI_File_set_view(fh, 0, etype, etype, "native", MPI_INFO_NULL);
    CHECK_ERROR(err, nerrs);

    err = MPI_Type_free(&etype);
    CHECK_ERROR(err, nerrs);

    /* create filetype with negative disp */
    disp[0] = -2;
    disp[1] = 2;
    blen[0] = 1;
    blen[1] = 1;
    err = MPI_Type_indexed(2, blen, disp, MPI_INT, &filetype);
    CHECK_ERROR(err, nerrs);

    err = MPI_Type_commit(&filetype);
    CHECK_ERROR(err, nerrs);

    err = MPI_File_set_view(fh, 0, MPI_INT, filetype, "native", MPI_INFO_NULL);
    CHECK_EXPECTED_ERROR(MPI_ERR_IO, err, nerrs);

    err = MPI_Type_free(&filetype);
    CHECK_ERROR(err, nerrs);

    /* create filetype with decreasing disp */
    disp[0] = 3;
    blen[0] = 1;
    disp[1] = 0;
    blen[1] = 1;
    err = MPI_Type_indexed(2, blen, disp, MPI_INT, &filetype);
    CHECK_ERROR(err, nerrs);

    err = MPI_Type_commit(&filetype);
    CHECK_ERROR(err, nerrs);

    err = MPI_File_set_view(fh, 0, MPI_INT, filetype, "native", MPI_INFO_NULL);
    CHECK_EXPECTED_ERROR(MPI_ERR_IO, err, nerrs);

    err = MPI_Type_free(&filetype);
    CHECK_ERROR(err, nerrs);

    /* create filetype with overlaps (should be OK for read-only) */
    disp[0] = 0;
    blen[0] = 3;
    disp[1] = 1;
    blen[1] = 1;
    err = MPI_Type_indexed(2, blen, disp, MPI_INT, &filetype);
    CHECK_ERROR(err, nerrs);

    err = MPI_Type_commit(&filetype);
    CHECK_ERROR(err, nerrs);

    err = MPI_File_set_view(fh, 0, MPI_INT, filetype, "native", MPI_INFO_NULL);
    CHECK_ERROR(err, nerrs);

    err = MPI_Type_free(&filetype);
    CHECK_ERROR(err, nerrs);

    err = MPI_File_close(&fh);
    CHECK_ERROR(err, nerrs);

    if (nerrs == 0)
        printf(" No Errors\n");
    MPI_Finalize();

    return (nerrs > 0);
}
