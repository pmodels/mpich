#ifndef DTYPES_H_INCLUDED
#define DTYPES_H_INCLUDED

void MTestDatatype2Generate(MPI_Datatype *, void **, void **, int *, int *, int *);
void MTestDatatype2Allocate(MPI_Datatype **, void ***, void ***, int **, int **, int *);
int MTestDatatype2Check(void *, void *, int);
int MTestDatatype2CheckAndPrint(void *, void *, int, char *, int);
void MTestDatatype2Free(MPI_Datatype *, void **, void **, int *, int *, int);
void MTestDatatype2BasicOnly(void);
#endif /* DTYPES_H_INCLUDED */
