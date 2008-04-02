/*

  throwtest.cxx

  This test was based on code provided with a bug report submitted by
  Eric Eidson edeidso@sandia.gov

*/

#include <mpi.h>
#include <iostream>

/* #define VERBOSE */

int main( int argc, char *argv[] )
{
    int errs = 0;
  
    MPI::Init();

    const unsigned int rank = MPI::COMM_WORLD.Get_rank();

    int errorClass = MPI::Add_error_class();
    int errorCode = MPI::Add_error_code(errorClass);
    MPI::Add_error_string(errorCode, "Internal-use Error Code");

    MPI::COMM_WORLD.Set_errhandler(MPI::ERRORS_THROW_EXCEPTIONS);
    
    try {
      if (rank == 0) {
	MPI::COMM_WORLD.Call_errhandler(errorCode);
	std::cerr << "Do Not See This\n";
	errs++;
      }
    }
    catch (MPI::Exception ex) {
#ifdef VERBOSE
    std::cerr << "MPI Exception: " << ex.Get_error_string() << std::endl;
#endif
    }
    catch (...) {
      std::cerr << "Caught Unknown Exception\n";
      errs++;
    }

    if (errs == 0) {
      std::cout << " No Errors" << std::endl;
    }
    else {
      std::cout << " Found " << errs << " errors" << std::endl;
    }

    MPI::Finalize();

    return 0;
}
