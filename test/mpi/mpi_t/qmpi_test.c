/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* This test checks that using QMPI works properly with two tools attached. This should cover
 * testing for any number of attached tools. */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpitest.h"
#include "mpitestconf.h"

static int test_val = 0;
static int qmpi_on = 0;

__attribute__ ((constructor))
void static_register_my_tool(void);

void test_init_function_pointer(int tool_id);
int Test_Init_Thread(QMPI_Context context, int tool_id, int *argc, char ***argv, int required,
                     int *provided);
int Test_Finalize(QMPI_Context context, int tool_id);
int Test_Recv(QMPI_Context context, int tool_id, void *buf, int count,
              MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status * status);
int Test_Send(QMPI_Context context, int tool_id, const void *buf, int count,
              MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);

struct tool_struct {
    int my_tool_id;
};
struct tool_struct my_tool_struct[2];
QMPI_Recv_t *next_recv_fn[2];
int next_recv_id[2];
QMPI_Send_t *next_send_fn[2];
int next_send_id[2];
QMPI_Finalize_t *next_finalize_fn[2];
int next_finalize_id[2];

__attribute__ ((constructor))
void static_register_my_tool(void)
{
    if (MPI_SUCCESS == QMPI_Register_tool_name("test_tool", &test_init_function_pointer)) {
        qmpi_on = 1;
    }
}

void test_init_function_pointer(int tool_id)
{
    my_tool_struct[tool_id - 1].my_tool_id = tool_id;

    QMPI_Register_tool_storage(tool_id, &my_tool_struct[tool_id - 1]);
    QMPI_Register_function(tool_id, MPI_INIT_THREAD_T, (void (*)(void)) &Test_Init_Thread);
    QMPI_Register_function(tool_id, MPI_FINALIZE_T, (void (*)(void)) &Test_Finalize);
    QMPI_Register_function(tool_id, MPI_SEND_T, (void (*)(void)) &Test_Send);
    QMPI_Register_function(tool_id, MPI_RECV_T, (void (*)(void)) &Test_Recv);
}

int Test_Init_Thread(QMPI_Context context, int tool_id, int *argc, char ***argv, int required,
                     int *provided)
{
    QMPI_Init_thread_t *next_init_thread_fn;
    int next_tool_id, ret;
    struct tool_struct *storage;

    QMPI_Get_tool_storage(context, tool_id, (void *) &storage);

    QMPI_Get_function(tool_id, MPI_INIT_THREAD_T, (void (**)(void)) &next_init_thread_fn,
                      &next_tool_id);
    QMPI_Get_function(tool_id, MPI_FINALIZE_T, (void (**)(void)) &next_finalize_fn[tool_id - 1],
                      &next_finalize_id[tool_id - 1]);
    QMPI_Get_function(tool_id, MPI_RECV_T, (void (**)(void)) &next_recv_fn[tool_id - 1],
                      &next_recv_id[tool_id - 1]);
    QMPI_Get_function(tool_id, MPI_SEND_T, (void (**)(void)) &next_send_fn[tool_id - 1],
                      &next_send_id[tool_id - 1]);

    ret = (*next_init_thread_fn) (context, next_tool_id, argc, argv, required, provided);

    return ret;
}

int Test_Finalize(QMPI_Context context, int tool_id)
{
    int ret;
    struct tool_struct *storage;

    QMPI_Get_tool_storage(context, tool_id, (void *) &storage);

    ret = (*next_finalize_fn[tool_id - 1]) (context, next_finalize_id[tool_id - 1]);

    return ret;
}

int Test_Recv(QMPI_Context context, int tool_id, void *buf, int count,
              MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status * status)
{
    int ret;
    struct tool_struct *storage;

    QMPI_Get_tool_storage(context, tool_id, (void *) &storage);

    test_val++;

    ret = (*next_recv_fn[tool_id - 1]) (context, next_recv_id[tool_id - 1], buf, count, datatype,
                                        source, tag, comm, status);

    return ret;
}

int Test_Send(QMPI_Context context, int tool_id, const void *buf, int count,
              MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
    int ret;
    struct tool_struct *storage;

    QMPI_Get_tool_storage(context, tool_id, (void *) &storage);

    test_val++;

    ret = (*next_send_fn[tool_id - 1]) (context, next_send_id[tool_id - 1], buf, count, datatype,
                                        dest, tag, comm);

    return ret;
}

int main(int argc, char *argv[])
{
    int errs = 0;
    int rank, size, send_buf = 1337, recv_buf;
    int expected_output = 4;

    if (!qmpi_on)
        expected_output = 0;

    MTest_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
        MPI_Send(&send_buf, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
        MPI_Recv(&recv_buf, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    } else {
        MPI_Recv(&recv_buf, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Send(&send_buf, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }

    if (recv_buf != 1337) {
        fprintf(stderr, "Received incorrect data: %d != 1337\n", recv_buf);
        errs++;
    }

    if (test_val != expected_output) {
        fprintf(stderr, "QMPI tool did not increment value: %d != 2\n", test_val);
        errs++;
    }

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
