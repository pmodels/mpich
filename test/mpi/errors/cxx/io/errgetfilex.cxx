/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*

  errgetx.cxx

  Tests whether a Get_errhandler following a Set_errhandler retrieves the same
  value as was set.
*/

#include "mpitestconf.h"
#include <mpi.h>
#include <iostream>
#include "mpitestcxx.h"

/* #define VERBOSE */

template <class T>
void efn(T &obj, int *, ...)
{
}

/* returns number of errors found */
template <class T>
int testRetrieveErrhandler(T &obj)
{
    int errs = 0;
    MPI::Errhandler retrieved_handler;

    int errorClass = MPI::Add_error_class();
    int errorCode = MPI::Add_error_code(errorClass);
    std::string errorString = "Internal-use Error Code";
    MPI::Add_error_string(errorCode, errorString.c_str());
    MPI::Errhandler custom_handler = T::Create_errhandler(&efn);

    obj.Set_errhandler(MPI::ERRORS_THROW_EXCEPTIONS);
    retrieved_handler = obj.Get_errhandler();
    if (retrieved_handler != MPI::ERRORS_THROW_EXCEPTIONS) errs++;

    obj.Set_errhandler(MPI::ERRORS_RETURN);
    retrieved_handler = obj.Get_errhandler();
    if (retrieved_handler != MPI::ERRORS_RETURN) errs++;

    obj.Set_errhandler(MPI::ERRORS_ARE_FATAL);
    retrieved_handler = obj.Get_errhandler();
    if (retrieved_handler != MPI::ERRORS_ARE_FATAL) errs++;

    obj.Set_errhandler(custom_handler);
    retrieved_handler = obj.Get_errhandler();
    if (retrieved_handler != custom_handler) errs++;
    retrieved_handler.Free();

    custom_handler.Free();

    return errs;
}

int main( int argc, char *argv[] )
{
    int errs = 0;
    MPI::File file = MPI::FILE_NULL;

    MTest_Init( );

    const unsigned int rank = MPI::COMM_WORLD.Get_rank();

    file = MPI::File::Open(MPI::COMM_WORLD, "testfile", 
	   MPI::MODE_WRONLY | MPI::MODE_CREATE | MPI::MODE_DELETE_ON_CLOSE, 
	   MPI::INFO_NULL);

    if (0 == rank) {
        errs += testRetrieveErrhandler(MPI::COMM_WORLD);
        errs += testRetrieveErrhandler(file);
    }

    MTest_Finalize( errs );

    file.Close();

    MPI::Finalize();

    return 0;
}
