#include "mpiimpl.h"
#include "scatter.h"

bool find_local_rank_linear(int* group, int group_size, int rank, int root, int* group_rank, int* group_root) {
    /*
        linear search for the group_rank
    */
    bool found_rank = 0;
    bool found_root = 0;

    for (int i = 0; i < group_size; i++) {
        if (group[i] == rank) {
            *group_rank = i;
            found_rank = 1; 
        }
        if (group[i] == root) {
            *group_root = i;
            found_root = 1;
        }
    }
    return found_rank && found_root;
}