#ifndef MPITEST_DTYPES
#define MPITEST_DTYPES

void MTestDatatype2Generate(MPI_Datatype *, void **, void **, int *, int *, int *);
void MTestDatatype2Allocate(MPI_Datatype **, void ***, void ***, int **, int **, int *);
int MTestDatatype2Check(void *, void *, int);
int MTestDatatype2CheckAndPrint(void *, void *, int, char *, int);
void MTestDatatype2Free(MPI_Datatype *, void **, void **, int *, int *, int);
void MTestDatatype2BasicOnly(void);
#endif
