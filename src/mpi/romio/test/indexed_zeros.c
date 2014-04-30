#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>

#include <mpi.h>

#define MAXLEN 9

static void handle_error(int errcode, const char *str)
{
	char msg[MPI_MAX_ERROR_STRING];
	int resultlen;
	MPI_Error_string(errcode, msg, &resultlen);
	fprintf(stderr, "%s: %s\n", str, msg);
	MPI_Abort(MPI_COMM_WORLD, 1);
}
int main(int argc, char **argv) {
    int i, rank, np, buflen, num, err, nr_errors;
    int  nelms[MAXLEN], buf[MAXLEN], indices[MAXLEN], blocklen[MAXLEN];
    MPI_File fh;
    MPI_Status status;
    MPI_Datatype filetype;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &np);

    if (np != 2) {
        if (rank == 0) fprintf(stderr,"Must run on 2 MPI processes\n");
        MPI_Finalize(); return 1;
    }

    /* set up the number of integers to write in each iteration */
    for (i=0; i<MAXLEN; i++) nelms[i] = 0;
    if (rank == 0) nelms[4]=nelms[5]=nelms[7]=1;
    if (rank == 1) nelms[0]=nelms[1]=nelms[2]=nelms[3]=nelms[6]=nelms[8]=1;

    /* pre-fill the file with integers -999 */
    if (rank == 0) {
        for (i=0; i<MAXLEN; i++) buf[i] = -999;
	err =MPI_File_open(MPI_COMM_SELF, argv[1], MPI_MODE_CREATE|MPI_MODE_WRONLY,
		MPI_INFO_NULL, &fh);
	if (err != MPI_SUCCESS) handle_error(err, "MPI_File_open");
        err = MPI_File_write(fh, buf, MAXLEN, MPI_INT, &status);
	if (err != MPI_SUCCESS) handle_error(err, "MPI_File_write");
        err = MPI_File_close(&fh);
	if (err != MPI_SUCCESS) handle_error(err, "MPI_File_close");
    }
    MPI_Barrier(MPI_COMM_WORLD);

    /* define a filetype using indexed constructor */
    buflen = num = 0;
    for (i=0; i<MAXLEN; i++) {
        buflen       += nelms[i];
        indices[num]  = i;
        blocklen[num] = nelms[i];
        num++;
    }
    MPI_Type_indexed(num, blocklen, indices, MPI_INT, &filetype);
    MPI_Type_commit(&filetype);

    /* initialize write buffer and write to file*/
    for (i=0; i<MAXLEN; i++) buf[i] = 1;
    err =MPI_File_open(MPI_COMM_WORLD, argv[1], MPI_MODE_WRONLY, MPI_INFO_NULL, &fh);
    if (err != MPI_SUCCESS) handle_error(err, "MPI_File_open");
    err = MPI_File_set_view(fh, 0, MPI_INT, filetype, "native", MPI_INFO_NULL);
    if (err != MPI_SUCCESS) handle_error(err, "MPI_File_set_view");
    err = MPI_File_write_all(fh, buf, buflen, MPI_INT, &status);
    if (err != MPI_SUCCESS) handle_error(err, "MPI_File_write_all");
    MPI_Type_free(&filetype);
    err = MPI_File_close(&fh);
    if (err != MPI_SUCCESS) handle_error(err, "MPI_File_close");

    /* read back and check */
    if (rank == 0) {
        err = MPI_File_open(MPI_COMM_SELF, argv[1], MPI_MODE_RDONLY, MPI_INFO_NULL, &fh);
	if (err != MPI_SUCCESS) handle_error(err, "MPI_File_open");
        err = MPI_File_read(fh,buf, MAXLEN, MPI_INT, &status);
	if (err != MPI_SUCCESS) handle_error(err, "MPI_File_read");
        err = MPI_File_close(&fh);
	if (err != MPI_SUCCESS) handle_error(err, "MPI_File_close");
        for (i=0; i<MAXLEN; i++) {
            if (buf[i] < 0) {
		nr_errors++;
                printf("Error: unexpected value at buf[%d] == %d\n",i,buf[i]);
	    }
	}
	if (nr_errors == 0) printf(" No Errors\n");
    }

    MPI_Finalize();
    return 0;
}

