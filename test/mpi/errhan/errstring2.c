#include <stdio.h>
#include <mpi.h>
int main(int argc, char *argv[])
{
    int errorclass;
    char errorstring[MPI_MAX_ERROR_STRING] = {64,0};
    int slen;
    MPI_Init(&argc, &argv);
    MPI_Add_error_class(&errorclass);
    MPI_Error_string(errorclass, errorstring, &slen);
    printf("errorclass:%d errorstring:'%s' len:%d\n", errorclass,
	    errorstring, slen);
    MPI_Finalize();
    return 0;
}
