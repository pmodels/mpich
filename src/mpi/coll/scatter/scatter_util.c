#include "mpiimpl.h"
#include "scatter.h"

bool find_local_rank(int* group, int group_size, int rank, int* group_rank) {
    int start = 0;
    int end = group_size;
    int middle = (start + end) / 2;

    while (start < end) {
        int mid_rank = group[middle];
        
        if (mid_rank == rank) {    
            if (group) *group_rank = middle;
             return 1;
        } else if (rank > mid_rank) {
            start = middle + 1;
        } else {
            end = middle;
        }
        middle = (start + end) / 2;
    }
    
    if (group) *group_rank = -1;
    return 0;
}