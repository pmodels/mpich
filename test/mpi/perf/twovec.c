/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h" 
#include <stdio.h>
#include <stdlib.h>

/* This does a transpose with a get operation, fence, and derived
   datatypes. Uses vector and hvector (Example 3.32 from MPI 1.1
   Standard). Run on 2 processes */

int main(int argc, char *argv[]) 
{ 
    MPI_Datatype column, xpose;
    double t[5], ttmp, tmin, tmax;
    static int sizes[5] = { 10, 100, 1000, 10000, 20000 };
    int i, isMonotone, errs=0, nrows, ncols;
 
    MPI_Init(&argc,&argv); 

     for (i=0; i<5; i++) {
         nrows = ncols = sizes[i];
         ttmp = MPI_Wtime();
         /* create datatype for one column */
         MPI_Type_vector(nrows, 1, ncols, MPI_INT, &column);
         /* create datatype for matrix in column-major order */
         MPI_Type_hvector(ncols, 1, sizeof(int), column, &xpose);
         MPI_Type_commit(&xpose);
         t[i] = MPI_Wtime() - ttmp;
         MPI_Type_free( &xpose );
         MPI_Type_free( &column );
     }

     /* Now, analyze the times to see that they are (a) small and (b)
        nearly independent of size */
     tmin = 10000;
     tmax = 0;
     for (i=0; i<5; i++) {
         if (t[i] < tmin) tmin = t[i];
         if (t[i] > tmax) tmax = t[i];
     }
     /* Monotone times are a warning */
     isMonotone = 1;
     for (i=1; i<5; i++) {
        if (t[i] < t[i-1]) isMonotone = 0;
     }
     if (tmax > 100 * tmin) {
         errs++;
         fprintf( stderr, "Range of times appears too large\n" );
         if (isMonotone) {
             fprintf( stderr, "Vector types may use processing proportion to count\n" );
         }
         for (i=0; i<5; i++) {
              fprintf( stderr, "n = %d, time = %f\n", sizes[i], t[i] );
         }
         fflush(stderr);
     }

    if (errs) {
        printf( " Found %d errors\n", errs );
    }
    else {
        printf( " No Errors\n" );
    } 
    MPI_Finalize(); 
    return 0; 
} 
