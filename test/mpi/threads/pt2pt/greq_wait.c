/* Based on test code provided by Lisandro Dalcí. */
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

static char MTEST_Descrip[] = "threaded generalized request tests";

#define IF_VERBOSE(a) \
do { \
    if (verbose) { \
        printf a ; \
        fflush(stdout); \
    } \
} while (0)

static int verbose = 0;

int query_fn(void *extra_state, MPI_Status *status)
{
  status->MPI_SOURCE = MPI_UNDEFINED;
  status->MPI_TAG    = MPI_UNDEFINED;
  MPI_Status_set_cancelled(status, 0);
  MPI_Status_set_elements(status, MPI_BYTE, 0);
  return 0;
}
int free_fn(void *extra_state) { return 0; }
int cancel_fn(void *extra_state, int complete) { return 0; }


MPI_Request grequest;

void *do_work(void *arg)
{
  MPI_Request *req = (MPI_Request *)arg;
  IF_VERBOSE(("Starting work in thread ...\n"));
  sleep(3);
  IF_VERBOSE(("Work in thread done !!!\n"));
  MPI_Grequest_complete(*req);
  return NULL;
}

int main(int argc, char *argv[])
{
    int provided;
    MPI_Request request;
    pthread_t thread; 
    int outcount = -1;
    int indices[1] = {-1};
    int errs;
    MPI_Status status;
    char *env;

    env = getenv("MPITEST_VERBOSE");
    if (env)
    {
        if (*env != '0')
            verbose = 1;
    }

    MPI_Init_thread( &argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    if (provided != MPI_THREAD_MULTIPLE) {
        printf( "This test requires MPI_THREAD_MULTIPLE\n" );
        MPI_Abort( MPI_COMM_WORLD, 1 );
    }

    IF_VERBOSE(("Post Init ...\n"));

    MPI_Grequest_start(query_fn, free_fn, cancel_fn, NULL, &request);
    grequest = request; /* copy the handle */
    pthread_create(&thread, NULL, do_work, &grequest);
    IF_VERBOSE(("Waiting ...\n"));
    MPI_Wait(&request, &status);
    pthread_join(thread, NULL);

    MPI_Grequest_start(query_fn, free_fn, cancel_fn, NULL, &request);
    grequest = request; /* copy the handle */
    pthread_create(&thread, NULL, do_work, &grequest);
    IF_VERBOSE(("Waiting ...\n"));
    MPI_Waitsome(1, &request, &outcount, indices, &status);
    pthread_join(thread, NULL);

    MPI_Grequest_start(query_fn, free_fn, cancel_fn, NULL, &request);
    grequest = request; /* copy the handle */
    pthread_create(&thread, NULL, do_work, &grequest);
    IF_VERBOSE(("Waiting ...\n"));
    MPI_Waitall(1, &request, &status);
    pthread_join(thread, NULL);

    IF_VERBOSE(("Goodbye !!!\n"));
    MTest_Finalize(0);
    MPI_Finalize();
    return 0;
}
