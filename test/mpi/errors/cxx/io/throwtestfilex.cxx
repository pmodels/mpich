/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*

  throwtest.cxx

  This test was based on code provided with a bug report submitted by
  Eric Eidson edeidso@sandia.gov

*/

#include <mpi.h>
#include <iostream>
#include "mpitestconf.h"
#include "mpitestcxx.h"

/* #define VERBOSE */

/* returns number of errors found */
template <class T>
int testCallErrhandler(T &obj, int errorClass, int errorCode, std::string errorString)
{
    int errs = 0;

    try {
        obj.Call_errhandler(errorCode);
        std::cerr << "Do Not See This" << std::endl;
        errs++;
    }
    catch (MPI::Exception &ex) {
#ifdef VERBOSE
        std::cerr << "MPI Exception: " << ex.Get_error_string() << std::endl;
#endif
        if (ex.Get_error_code() != errorCode) {
            std::cout << "errorCode does not match" << std::endl;
            errs++;
        }
        if (ex.Get_error_class() != errorClass) {
            std::cout << "errorClass does not match" << std::endl;
            errs++;
        }
        if (ex.Get_error_string() != errorString) {
            std::cout << "errorString does not match" << std::endl;
            errs++;
        }
    }
    catch (...) {
        std::cerr << "Caught Unknown Exception" << std::endl;
        errs++;
    }

    return errs;
}

int main( int argc, char *argv[] )
{
    int errs = 0;
    MPI::File file = MPI::FILE_NULL;

    MTest_Init( );

    const unsigned int rank = MPI::COMM_WORLD.Get_rank();
    const unsigned int size = MPI::COMM_WORLD.Get_size();

    int errorClass = MPI::Add_error_class();
    int errorCode = MPI::Add_error_code(errorClass);
    std::string errorString = "Internal-use Error Code";
    MPI::Add_error_string(errorCode, errorString.c_str());

    file = MPI::File::Open(MPI::COMM_WORLD, "testfile", 
	   MPI::MODE_WRONLY | MPI::MODE_CREATE | MPI::MODE_DELETE_ON_CLOSE, 
	   MPI::INFO_NULL);

    // first sanity check that ERRORS_RETURN actually returns in erroneous
    // conditions and doesn't throw an exception
    MPI::COMM_WORLD.Set_errhandler(MPI::ERRORS_RETURN);
    file.Set_errhandler(MPI::ERRORS_RETURN);

    try {
        // Do something that should cause an exception.
        file.Write(NULL, -1, MPI_DATATYPE_NULL);
    }
    catch (...) {
        std::cerr << "file threw when it shouldn't have" << std::endl;
        ++errs;
    }

    // now test that when ERRORS_THROW_EXCEPTIONS actually throws an exception
    MPI::COMM_WORLD.Set_errhandler(MPI::ERRORS_THROW_EXCEPTIONS);
    file.Set_errhandler(MPI::ERRORS_THROW_EXCEPTIONS);

    if (0 == rank) {
        errs += testCallErrhandler(MPI::COMM_WORLD, errorClass, errorCode, errorString);
        errs += testCallErrhandler(file,            errorClass, errorCode, errorString);

        try {
            int buf[10];
            file.Write_at(-2, &buf, 1, MPI::INT);
            std::cout << "Invalid Send did not throw" << std::endl;
            errs++;
        }
        catch (MPI::Exception &ex) {
            // expected
        }
        catch (...) {
            std::cout << "Caught Unknown Exception" << std::endl;
            errs++;
        }
    }

    MTest_Finalize( errs );

    file.Close();

    MPI::Finalize();

    return 0;
}
