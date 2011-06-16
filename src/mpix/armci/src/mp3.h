#ifndef _MP3_H_
#define _MP3_H_

#include <mpi.h>

#define MP_INIT(ARGC,ARGV)   MPI_Init(&(ARGC),&(ARGV))
#define MP_BARRIER()         MPI_Barrier(MPI_COMM_WORLD)
#define MP_FINALIZE()        MPI_Finalize()
#define MP_PROCS(X)          MPI_Comm_size(MPI_COMM_WORLD,X)
#define MP_MYID(X)           MPI_Comm_rank(MPI_COMM_WORLD,X)
#define MP_TIMER()           MPI_Wtime()

#endif /* _MP3_H_ */
