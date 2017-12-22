#!/bin/sh
#
# (C) 2018 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#
nl='
'

coll_algo_tests=""

########## Add tests for bcast algorithms ############

#disable device collectives for bcast to test MPIR algorithms
testing_env="env=MPIR_CVAR_BCAST_DEVICE_COLLECTIVE=0 "

#test nb algorithms
testing_env+="env=MPIR_CVAR_BCAST_INTRA_ALGORITHM=nb "
testing_env+="env=MPIR_CVAR_IBCAST_DEVICE_COLLECTIVE=0 "
algo_names="tree_kary tree_knomial"
kvalues="3"

for algo_name in ${algo_names}; do
    for kval in ${kvalues}; do
        #set the environment
        env="${testing_env} env=MPIR_CVAR_IBCAST_INTRA_ALGORITHM=${algo_name} "
        env+="env=MPIR_CVAR_IBCAST_TREE_KVAL=${kval} env=MPIR_CVAR_IBCAST_TREE_PIPELINE_CHUNK_SIZE=4096 "

        coll_algo_tests+="bcasttest 10 ${env}${nl}"
        coll_algo_tests+="bcastzerotype 5 ${env}${nl}"
        coll_algo_tests+="bcasttest 10 ${env}${nl}"
    done
done

########## Add tests for reduce algorithms ############

#disable device collectives for reduce to test MPIR algorithms
testing_env="env=MPIR_CVAR_REDUCE_DEVICE_COLLECTIVE=0 "

#test nb algorithms
testing_env+="env=MPIR_CVAR_REDUCE_INTRA_ALGORITHM=nb "
testing_env+="env=MPIR_CVAR_IREDUCE_DEVICE_COLLECTIVE=0 "
algo_names="tree_kary tree_knomial ring"
kvalues="3"

for algo_name in ${algo_names}; do
    for kval in ${kvalues}; do
        #set the environment
        env="${testing_env} env=MPIR_CVAR_IREDUCE_INTRA_ALGORITHM=${algo_name} "
        env+="env=MPIR_CVAR_IREDUCE_TREE_KVAL=${kval} env=MPIR_CVAR_IREDUCE_TREE_PIPELINE_CHUNK_SIZE=4096 "

        coll_algo_tests+="reduce 5 ${env}${nl}"
        coll_algo_tests+="reduce 10 ${env}${nl}"
        coll_algo_tests+="red3 10 ${env}${nl}"
        coll_algo_tests+="red4 10 ${env}${nl}"
    done
done

export coll_algo_tests
