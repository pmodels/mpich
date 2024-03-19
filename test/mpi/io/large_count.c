
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <mpi.h>


static int Errors;

#define ERROR(fname) \
    if (err != MPI_SUCCESS) { \
        int errorStringLen; \
        char errorString[MPI_MAX_ERROR_STRING]; \
        MPI_Error_string(err, errorString, &errorStringLen); \
	Errors++; \
        printf("Error at line %d when calling %s: %s\n",__LINE__,fname,errorString); \
    }

#define LEN 2048
#define NVARS 1100

/*----< main() >------------------------------------------------------------*/
int main(int argc, char **argv)
{
    char *filename;
    size_t i, buf_len;
    int err, rank, verbose = 1, nprocs, psize[2], gsize[2], count[2], start[2];
    char *buf;
    MPI_File fh;
    MPI_Datatype subType, filetype, buftype;
    MPI_Status status;
    MPI_Offset fsize;
    int array_of_blocklengths[NVARS];
    MPI_Aint array_of_displacements[NVARS];
    MPI_Datatype array_of_types[NVARS];

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    if (argc != 2) {
        if (!rank)
            printf("Usage: %s filename\n", argv[0]);
        MPI_Finalize();
        exit(1);
    }
    filename = argv[1];

    /* Creates a division of processors in a cartesian grid */
    psize[0] = psize[1] = 0;
    err = MPI_Dims_create(nprocs, 2, psize);
    ERROR("MPI_Dims_create");

    /* each 2D variable is of size gsizes[0] x gsizes[1] bytes */
    gsize[0] = LEN * psize[0];
    gsize[1] = LEN * psize[1];

    /* set subarray offset and length */
    start[0] = LEN * (rank / psize[1]);
    start[1] = LEN * (rank % psize[1]);
    count[0] = LEN - 1; /* -1 to create holes */
    count[1] = LEN - 1;

    fsize = (MPI_Offset) gsize[0] * gsize[1] * NVARS - (LEN + 1);
    if (verbose) {
        buf_len = (size_t) NVARS *(LEN - 1) * (LEN - 1);
        if (rank == 0) {
            printf("nprocs=%d NVARS=%d LEN=%d\n", nprocs, NVARS, LEN);
            printf("Expecting file size=%lld bytes (%.1f MB, %.1f GB)\n",
                   fsize, (float) fsize / 1048576, (float) fsize / 1073741824);
            printf("Each global variable is of size %d bytes (%.1f MB)\n",
                   gsize[0] * gsize[1], (float) gsize[0] * gsize[1] / 1048576);
            printf("Each process writes %zd bytes (%.1f MB, %.1f GB)\n",
                   buf_len, (float) buf_len / 1048576, (float) buf_len / 1073741824);
        }
        printf("rank %3d: gsize=%4d %4d start=%4d %4d count=%4d %4d\n", rank,
               gsize[0], gsize[1], start[0], start[1], count[0], count[1]);
    }


    /* create 2D subarray datatype for fileview */
    err = MPI_Type_create_subarray(2, gsize, count, start, MPI_ORDER_C, MPI_BYTE, &subType);
    ERROR("MPI_Type_create_subarray");
    err = MPI_Type_commit(&subType);
    ERROR("MPI_Type_commit");

    /* create a filetype by concatenating NVARS subType */
    for (i = 0; i < NVARS; i++) {
        array_of_blocklengths[i] = 1;
        array_of_displacements[i] = gsize[0] * gsize[1] * i;
        array_of_types[i] = subType;
    }
    err = MPI_Type_create_struct(NVARS, array_of_blocklengths,
                                 array_of_displacements, array_of_types, &filetype);
    ERROR("MPI_Type_create_struct");
    err = MPI_Type_commit(&filetype);
    ERROR("MPI_Type_commit");
    err = MPI_Type_free(&subType);
    ERROR("MPI_Type_free");

    /* Create local buffer datatype: each 2D variable is of size LEN x LEN */
    gsize[0] = LEN;
    gsize[1] = LEN;
    start[0] = 0;
    start[1] = 0;
    count[0] = LEN - 1; /* -1 to create holes */
    count[1] = LEN - 1;

    err = MPI_Type_create_subarray(2, gsize, count, start, MPI_ORDER_C, MPI_BYTE, &subType);
    ERROR("MPI_Type_create_subarray");
    err = MPI_Type_commit(&subType);
    ERROR("MPI_Type_commit");

    /* concatenate NVARS subType into a buftype */
    for (i = 0; i < NVARS; i++) {
        array_of_blocklengths[i] = 1;
        array_of_displacements[i] = LEN * LEN * i;
        array_of_types[i] = subType;
    }

    /* create a buftype by concatenating NVARS subTypes */
    err = MPI_Type_create_struct(NVARS, array_of_blocklengths,
                                 array_of_displacements, array_of_types, &buftype);
    ERROR("MPI_Type_create_struct");
    err = MPI_Type_commit(&buftype);
    ERROR("MPI_Type_commit");
    err = MPI_Type_free(&subType);
    ERROR("MPI_Type_free");

    /* allocate a local buffer */
    buf_len = (size_t) NVARS *LEN * LEN;
    buf = (char *) malloc(buf_len);
    for (i = 0; i < buf_len; i++)
        buf[i] = (char) rank;

    /* open to create a file */
    err =
        MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_CREATE | MPI_MODE_RDWR, MPI_INFO_NULL,
                      &fh);
    ERROR("MPI_File_open");

    /* set the file view */
    err = MPI_File_set_view(fh, 0, MPI_BYTE, filetype, "native", MPI_INFO_NULL);
    ERROR("MPI_File_set_view");

    /* MPI collective write */
    err = MPI_File_write_all(fh, buf, 1, buftype, &status);
    ERROR("MPI_File_write_all");

    /* do it again but use the non-blocking collectives */
    err = MPI_File_seek(fh, 0, MPI_SEEK_SET);
    ERROR("MPI_File_seek");
    MPI_Request  req;
    err = MPI_File_iwrite_all(fh, buf, 1, buftype, &req);
    ERROR("MPI_File_write_all");
    MPI_Wait(&req, &status);

    err = MPI_File_read_all(fh, buf, 1, buftype, &status);
    ERROR("MPI_File_read_all");

    err = MPI_File_seek(fh, 0, MPI_SEEK_SET);
    ERROR("MPI_File_seek");
    err = MPI_File_iread_all(fh, buf, 1, buftype, &req);
    MPI_Wait(&req, &status);


    MPI_File_close(&fh);
    free(buf);

    err = MPI_Type_free(&filetype);
    ERROR("MPI_Type_free");
    err = MPI_Type_free(&buftype);
    ERROR("MPI_Type_free");

    if (Errors > 0) {
        printf(" Found %d errors\n", Errors);
    } else {
        printf(" No Errors\n");
    }

    MPI_Finalize();
    return 0;
}
