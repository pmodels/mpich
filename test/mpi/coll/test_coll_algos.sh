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
        env+="env=MPIR_CVAR_IBCAST_TREE_KVAL=${kval}"

        coll_algo_tests+="bcasttest 10 ${env}${nl}"
        coll_algo_tests+="bcast_full 4 timeLimit=600 ${env}${nl}"
        coll_algo_tests+="bcast_min_datatypes 10 timeLimit=1200 ${env}${nl}"
        coll_algo_tests+="bcast_comm_world 10 timeLimit=1200 ${env}${nl}"
        coll_algo_tests+="bcastzerotype 5 ${env}${nl}"
        coll_algo_tests+="bcasttest 10 ${env} env=MPIR_CVAR_IBCAST_TREE_PIPELINE_CHUNK_SIZE=4096 ${nl}"
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
######### Add tests for Allreduce algorithms ###########

#disable device collectives for allreduce to test MPIR algorithms
testing_env="env=MPIR_CVAR_ALLREDUCE_DEVICE_COLLECTIVE=0 "

#test nb algorithms
testing_env+="env=MPIR_CVAR_ALLREDUCE_INTRA_ALGORITHM=nb "
testing_env+="env=MPIR_CVAR_IALLREDUCE_DEVICE_COLLECTIVE=0 "
algo_names="recexch_single_buffer recexch_multiple_buffer"
kvalues="2 3 4"

for algo_name in ${algo_names}; do
    for kval in ${kvalues}; do
        #set the environment
        env="${testing_env} env=MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM=${algo_name} "
        env+="env=MPIR_CVAR_IALLREDUCE_RECEXCH_KVAL=${kval}"

        coll_algo_tests+="allred 4 arg=100 ${env}${nl}"
        coll_algo_tests+="allred 7 ${env}${nl}"
        coll_algo_tests+="allredmany 4 ${env}${nl}"
        coll_algo_tests+="allred2 4 ${env}${nl}"
        coll_algo_tests+="allred3 10 ${env}${nl}"
        coll_algo_tests+="allred4 4 ${env}${nl}"
        coll_algo_tests+="allred5 5 ${env}${nl}"
        coll_algo_tests+="allred6 4 ${env}${nl}"
        coll_algo_tests+="allred6 7 ${env}${nl}"
    done
done

######### Add tests for Allgather algorithms ###########

#disable device collectives for allgather to test MPIR algorithms
testing_env="env=MPIR_CVAR_ALLGATHER_DEVICE_COLLECTIVE=0 "

#test nb algorithms
testing_env+="env=MPIR_CVAR_ALLGATHER_INTRA_ALGORITHM=nb "
testing_env+="env=MPIR_CVAR_IALLGATHER_DEVICE_COLLECTIVE=0 "
algo_names="recexch_distance_doubling recexch_distance_halving"
kvalues="2 3 4"

for algo_name in ${algo_names}; do
    for kval in ${kvalues}; do
        #set the environment
        env="${testing_env} env=MPIR_CVAR_IALLGATHER_INTRA_ALGORITHM=${algo_name} "
        env+="env=MPIR_CVAR_IALLGATHER_RECEXCH_KVAL=${kval}"

        coll_algo_tests+="allgather2 10 ${env}${nl}"
        coll_algo_tests+="allgather3 10 ${env}${nl}"
    done
done

export coll_algo_tests
