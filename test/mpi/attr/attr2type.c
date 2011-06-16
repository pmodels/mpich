/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2007 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

static int foo_keyval = MPI_KEYVAL_INVALID;

int foo_initialize(void);
void foo_finalize(void);

int foo_copy_attr_function(MPI_Datatype type, int type_keyval,
			   void *extra_state, void *attribute_val_in,
			   void *attribute_val_out, int *flag);
int foo_delete_attr_function(MPI_Datatype type, int type_keyval,
			     void *attribute_val, void *extra_state);
static const char *my_func = 0;
static int verbose = 0;
static int delete_called = 0;
static int copy_called = 0;

int main(int argc, char *argv[])
{
    int mpi_errno;
    MPI_Datatype type, duptype;
    int rank;

    MPI_Init(&argc, &argv);
    
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );

    foo_initialize();

    mpi_errno = MPI_Type_contiguous(2, MPI_INT, &type);

    mpi_errno = MPI_Type_set_attr(type, foo_keyval, NULL);

    mpi_errno = MPI_Type_dup(type, &duptype);

    my_func = "Free of type";
    mpi_errno = MPI_Type_free(&type);

    my_func = "free of duptype";
    mpi_errno = MPI_Type_free(&duptype);

    foo_finalize();

    if (rank == 0) {
      int errs = 0;
      if (copy_called != 1) {
	printf( "Copy called %d times; expected once\n", copy_called );
	errs++;
      }
      if (delete_called != 2) {
	printf( "Delete called %d times; expected twice\n", delete_called );
	errs++;
      }
      if (errs == 0) {
	printf( " No Errors\n" );
      }
      else {
	printf( " Found %d errors\n", errs );
      }
      fflush(stdout);
    }

    MPI_Finalize();
    return 0;
}

int foo_copy_attr_function(MPI_Datatype type,
			   int type_keyval,
			   void *extra_state,
			   void *attribute_val_in,
			   void *attribute_val_out,
			   int *flag)
{
    if (verbose) printf("copy fn. called\n");
    copy_called ++;
    * (char **) attribute_val_out = NULL;
    *flag = 1;

    return MPI_SUCCESS;
}

int foo_delete_attr_function(MPI_Datatype type,
			     int type_keyval,
			     void *attribute_val,
			     void *extra_state)
{
    if (verbose) printf("delete fn. called in %s\n", my_func );
    delete_called ++;

    return MPI_SUCCESS;
}

int foo_initialize(void)
{
    int mpi_errno;

    /* create keyval for use later */
    mpi_errno = MPI_Type_create_keyval(foo_copy_attr_function,
				       foo_delete_attr_function,
				       &foo_keyval,
				       NULL);
    if (verbose) printf("created keyval\n");

    return 0;
}

void foo_finalize(void)
{
    int mpi_errno;

    /* remove keyval */
    mpi_errno = MPI_Type_free_keyval(&foo_keyval);

    if (verbose) printf("freed keyval\n");

    return;
}
