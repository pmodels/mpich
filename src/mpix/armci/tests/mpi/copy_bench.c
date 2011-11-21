#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mpi.h>

#define NITER 100000
#define MAXSZ 65536*2*2

int main(int argc, char **argv) {
  int      i, j;
  char    *in, *out;
  double   t_start, t_stop;
  MPI_Comm copy_comm;

  MPI_Init(&argc, &argv);

  MPI_Comm_dup(MPI_COMM_SELF, &copy_comm);

  in  = malloc(MAXSZ);
  out = malloc(MAXSZ);

  for (i = 0; i < MAXSZ; i++) {
    in[i]  = 0xAA;
    out[i] = 0x55;
  }

  for (i = 1; i <= MAXSZ; i *= 2) {
    t_start = MPI_Wtime();
    for (j = 0; j < NITER; j++) {
      memcpy(out, in, i);
    }
    t_stop = MPI_Wtime();
    printf("MEMCPY: %7d\t%0.9f\n", i, t_stop-t_start);
  }

  for (i = 0; i < MAXSZ; i++) {
    in[i]  = 0xAA;
    out[i] = 0x55;
  }

  for (i = 1; i <= MAXSZ; i *= 2) {
    t_start = MPI_Wtime();
    for (j = 0; j < NITER; j++) {
      MPI_Sendrecv(in, i, MPI_BYTE,
          0 /* rank */, 0 /* tag */,
          out, i, MPI_BYTE,
          0 /* rank */, 0 /* tag */,
          copy_comm, MPI_STATUS_IGNORE);
    }
    t_stop = MPI_Wtime();
    printf("SNDRCV: %7d\t%0.9f\n", i, t_stop-t_start);
  }


  MPI_Comm_free(&copy_comm);
  MPI_Finalize();

  return 0;
}
