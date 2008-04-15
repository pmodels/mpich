/*

  throwtest.cxx

  This test was based on code provided with a bug report submitted by
  Eric Eidson edeidso@sandia.gov

*/

#include <mpi.h>
#include <iostream>

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
    MPI::Win win = MPI::WIN_NULL;
    MPI::File file = MPI::FILE_NULL;

    MPI::Init();

    const unsigned int rank = MPI::COMM_WORLD.Get_rank();

    int errorClass = MPI::Add_error_class();
    int errorCode = MPI::Add_error_code(errorClass);
    std::string errorString = "Internal-use Error Code";
    MPI::Add_error_string(errorCode, errorString.c_str());

    win = MPI::Win::Create(NULL, 0, 1, MPI::INFO_NULL, MPI_COMM_WORLD);
    file = MPI::File::Open(MPI::COMM_WORLD, "testfile", MPI::MODE_WRONLY | MPI::MODE_CREATE | MPI::MODE_DELETE_ON_CLOSE, MPI::INFO_NULL);

    MPI::COMM_WORLD.Set_errhandler(MPI::ERRORS_THROW_EXCEPTIONS);
    win.Set_errhandler(MPI::ERRORS_THROW_EXCEPTIONS);
    file.Set_errhandler(MPI::ERRORS_THROW_EXCEPTIONS);

    if (0 == rank) {
        errs += testCallErrhandler(MPI::COMM_WORLD, errorClass, errorCode, errorString);
        errs += testCallErrhandler(win,             errorClass, errorCode, errorString);
        errs += testCallErrhandler(file,            errorClass, errorCode, errorString);
    }

    if (errs == 0) {
        std::cout << " No Errors" << std::endl;
    }
    else {
        std::cout << " Found " << errs << " errors" << std::endl;
    }

    win.Free();
    file.Close();

    MPI::Finalize();

    return 0;
}
