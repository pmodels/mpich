#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>

#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define LIN(x, y, dimX) ((x) + (y)*(dimX))
#define IDX(A, row, col) ((A)->M[LIN((col), (row), (A)->numCols)])

typedef struct {
    double *M;
    int numRows, numCols;
} Matrix;
void cleanup(Matrix *A) { free(A->M); free(A); }

void naive(Matrix *C, const Matrix *A, const Matrix *B, int row0, int rowMax) {
    for (int i = 0; i < C->numRows; ++i) {
        int row = i + row0;
        for (int col = 0; col < B->numCols; ++col)
            for (int j = 0; j < A->numCols; ++j)
                IDX(C, i, col) += IDX(A, row, j) * IDX(B, j, col);
    }
}

void notSoNaive(Matrix *C, const Matrix *A, const Matrix *B, int row0, int rowMax) {
    for (int k = 0; k < A->numCols; k++) {
        for (int i = 0; i < C->numRows; i++) {
            int y = i + row0;
            double r = IDX(A, y, k);
            for (int j = 0; j < B->numCols; j++)
                IDX(C, i, j) += r * IDX(B, k, j);
        }
    }
}

Matrix *doWork(const Matrix *A, const Matrix *B, int myRank, int numRanks) {
    // Number of C rows for which this rank is responsible.
    int numRowsResponsible = A->numRows / numRanks;
    int row0 = myRank * numRowsResponsible;
    int rowMax = MIN((myRank+1) * numRowsResponsible, A->numRows);

    Matrix *myC = malloc(sizeof(Matrix));
    myC->numRows = rowMax - row0;
    myC->numCols = B->numCols;
    myC->M = calloc(myC->numRows * myC->numCols, sizeof(double));

    // Multiply the matrices.
    printf("numRows: %d \t numCols: %d\n", myC->numRows, myC->numCols);
    naive(myC, A, B, row0, rowMax);

    return myC;
}

void collect(Matrix *myC, int myRank, int productSize) {
    Matrix *C;
    if (myRank == 0) {
        C = malloc(sizeof(Matrix));
        C->M = malloc(productSize * sizeof(double));
        C->numCols = myC->numCols;
        C->numRows = productSize / myC->numCols;
    }

    // TODO: Might want to use a gatherv instead.
    MPI_Gather(myC->M, myC->numRows * myC->numCols, MPI_DOUBLE,
                 C->M, myC->numRows * myC->numCols, MPI_DOUBLE,
               0, MPI_COMM_WORLD);

    printf("%f\n", C[0]);
    /* if (myRank == 0) cleanup(C); */
}

Matrix *randMatrix(int numRows, int numCols) {
    Matrix *r = malloc(sizeof(Matrix));
    r->numRows = numRows; r->numCols = numCols;
    r->M = malloc(numRows * numCols * sizeof(double));

    for (int n = 0; n < numRows*numCols; ++n)
        r->M[n] = ((double) rand())/RAND_MAX;

    return r;
}

int main(int argc, char *argv[]) {
    int numRanks, myRank;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
    MPI_Comm_size(MPI_COMM_WORLD, &numRanks);

    // Create random matrices.
    Matrix *A = randMatrix(atoi(argv[1]), atoi(argv[2]));
    Matrix *B = randMatrix(atoi(argv[2]), atoi(argv[3]));

    // Delegate work.
    Matrix *myC = doWork(A, B, myRank, numRanks);

    // Collect & merge results.
    collect(myC, myRank, A->numRows * B->numCols);

    cleanup(A);
    cleanup(B);
    cleanup(myC);
    MPI_Finalize();

    return 0;
}

