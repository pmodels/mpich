/* CC: nvcc -g */
/* lib_list: -lmpi */
/* run: mpirun -l -n 2 */

#include <mpi.h>
#include <stdio.h>
#include <assert.h>

__global__
void saxpy(int n, float a, float *x, float *y)
{
  int i = blockIdx.x*blockDim.x + threadIdx.x;
  if (i < n) y[i] = a*x[i] + y[i];
}

int main(void)
{
    int mpi_errno;
    int rank, size;
    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    cudaStream_t stream;
    cudaStreamCreate(&stream);

    int N = 1000000;
    float *x, *y, *d_x, *d_y;
    x = (float*)malloc(N*sizeof(float));
    y = (float*)malloc(N*sizeof(float));

    cudaMalloc(&d_x, N*sizeof(float)); 
    cudaMalloc(&d_y, N*sizeof(float));

    if (rank == 0) {
        for (int i = 0; i < N; i++) {
            x[i] = 1.0f;
        }
    } else if (rank == 1) {
        for (int i = 0; i < N; i++) {
            y[i] = 2.0f;
        }
    }

    if (rank == 0) {
      #if 0  
        cudaMemcpyAsync(d_x, x, N*sizeof(float), cudaMemcpyHostToDevice, stream);

        mpi_errno = MPIX_Send_enqueue(d_x, N, MPI_FLOAT, 1, 0, MPI_COMM_WORLD, &stream);
      #else  
        mpi_errno = MPIX_Send_enqueue(x, N, MPI_FLOAT, 1, 0, MPI_COMM_WORLD, &stream);
      #endif  
        assert(mpi_errno == MPI_SUCCESS);

        cudaStreamSynchronize(stream);
    } else if (rank == 1) {
        cudaMemcpyAsync(d_y, y, N*sizeof(float), cudaMemcpyHostToDevice, stream);

        mpi_errno = MPIX_Recv_enqueue(d_x, N, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE, &stream);
        assert(mpi_errno == MPI_SUCCESS);

        // Perform SAXPY on 1M elements
        saxpy<<<(N+255)/256, 256, 0, stream>>>(N, 2.0f, d_x, d_y);

        cudaMemcpyAsync(y, d_y, N*sizeof(float), cudaMemcpyDeviceToHost, stream);

        cudaStreamSynchronize(stream);
    }

    if (rank == 1) {
        float maxError = 0.0f;
        int errs = 0;
        for (int i = 0; i < N; i++) {
            if (abs(y[i] - 4.0f) > 0.01) {
                errs++;
                maxError = max(maxError, abs(y[i]-4.0f));
            }
        }
        if (errs > 0) {
            printf("%d errors, Max error: %f\n", errs, maxError);
        } else {
            printf("No Errors\n");
        }
    }

    cudaFree(d_x);
    cudaFree(d_y);
    free(x);
    free(y);

    cudaStreamDestroy(stream);
    MPI_Finalize();
}
