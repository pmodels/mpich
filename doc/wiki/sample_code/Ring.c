#include "mpi.h"
#include "stdio.h"
#include "stdlib.h"

#define LOOP_COUNT 10

void linear(MPI_Comm intracomm, int injectionPoint) {
    int lineRank, lineSize, message;
    MPI_Status status;  // eventually we have to start checking this...
    MPI_Comm intercomm;

    MPI_Comm_rank(intracomm, &lineRank);
    MPI_Comm_size(intracomm, &lineSize);

    MPI_Intercomm_create(intracomm, 0, MPI_COMM_WORLD, injectionPoint, 99, &intercomm);

    if (lineRank == lineSize-1) {
        // Injection point.
        MPI_Recv(&message,  1, MPI_INT, lineRank-1, 1, intracomm, &status);
        MPI_Send(&lineRank, 1, MPI_INT, 0, 2, intercomm);
    } else if (lineRank == 0) {
        // Line entrance.
        MPI_Send(&lineRank, 1, MPI_INT, lineRank+1, 1, intracomm);
    } else {
        // Rest of the line.
        MPI_Recv(&message,  1, MPI_INT, lineRank-1, 1, intracomm, &status);
        MPI_Send(&lineRank, 1, MPI_INT, lineRank+1, 1, intracomm);
    }

    MPI_Comm_free(&intercomm);
}

void loop(MPI_Comm intracomm, int injectionPoint) {
    int i;
    int ringRank, ringSize;
    int origin, dest, message;
    MPI_Status status;  // eventually we have to start checking this...
    MPI_Comm intercomm;

    int buf[10000] = { 10 };

    MPI_Comm_rank(intracomm, &ringRank);
    MPI_Comm_size(intracomm, &ringSize);

    MPI_Intercomm_create(intracomm, 0, MPI_COMM_WORLD, 0, 99, &intercomm);

    if (ringRank == 0) {
        origin = injectionPoint-1;
        MPI_Recv(buf, 10, MPI_INT, origin, 2, intercomm, &status);

        message = ringRank;
        MPI_Send(buf, 10, MPI_INT, (ringRank + 1) % ringSize, 2, intracomm);
    }

    dest = (ringRank + 1) % ringSize;
    origin = (ringRank + ringSize - 1) % ringSize;
    for (i = 0; i < LOOP_COUNT; ++i) {
        MPI_Recv(buf, 10, MPI_INT, origin, 2, intracomm, &status);

        message = ringRank;
        MPI_Send(buf, 10, MPI_INT, dest, 2, intracomm);
    }

    MPI_Comm_free(&intercomm);
}

int main(int argc, char *argv[])
{
    int i, color;
    int rank, nprocs, injectionPoint;
    MPI_Comm intracomm;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    // Divide ranks into injection line & ring.
    injectionPoint = atoi(argv[1]);
    color = (rank < injectionPoint) ? 0 : 1;
    MPI_Comm_split(MPI_COMM_WORLD, color, rank, &intracomm);

    // First send message down the linear path, then inject it into the ring.
    if (rank < injectionPoint) {
        linear(intracomm, injectionPoint);
    } else {
        loop(intracomm, injectionPoint);
    }

    MPI_Comm_free(&intracomm);
    MPI_Finalize();

    return 0;
}
