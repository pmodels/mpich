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
algo_names="tree scatter_recexch_allgather ring"
tree_types="kary knomial_1 knomial_2"
kvalues="3"

for algo_name in ${algo_names}; do
    for kval in ${kvalues}; do
        if [ ${algo_name} = "tree" ]; then
            for tree_type in ${tree_types}; do
                #set the environment
                env="${testing_env} env=MPIR_CVAR_IBCAST_INTRA_ALGORITHM=${algo_name} "
                env+="env=MPIR_CVAR_IBCAST_TREE_TYPE=${tree_type} "
                env+="env=MPIR_CVAR_IBCAST_TREE_KVAL=${kval} env=MPIR_CVAR_IBCAST_TREE_PIPELINE_CHUNK_SIZE=4096 "
                env+="env=MPIR_CVAR_IBCAST_RING_CHUNK_SIZE=4096 "

                coll_algo_tests+="bcasttest 10 ${env}${nl}"
                coll_algo_tests+="bcastzerotype 5 ${env}${nl}"
            done
        else
            #set the environment
            env="${testing_env} env=MPIR_CVAR_IBCAST_INTRA_ALGORITHM=${algo_name} "
            env+="env=MPIR_CVAR_IBCAST_SCATTER_KVAL=${kval} env=MPIR_CVAR_IBCAST_ALLGATHER_RECEXCH_KVAL=${kval} "

            coll_algo_tests+="bcasttest 10 ${env}${nl}"
            coll_algo_tests+="bcastzerotype 5 ${env}${nl}"
        fi
    done
done

########## Add tests for reduce algorithms ############

#disable device collectives for reduce to test MPIR algorithms
testing_env="env=MPIR_CVAR_REDUCE_DEVICE_COLLECTIVE=0 "

#test nb algorithms
testing_env+="env=MPIR_CVAR_REDUCE_INTRA_ALGORITHM=nb "
testing_env+="env=MPIR_CVAR_IREDUCE_DEVICE_COLLECTIVE=0 "
algo_names="tree ring"
tree_types="kary knomial_1 knomial_2"
kvalues="3"

for algo_name in ${algo_names}; do
    if [ ${algo_name} = "tree" ]; then
        for tree_type in ${tree_types}; do
            for kval in ${kvalues}; do
                #set the environment
                env="${testing_env} env=MPIR_CVAR_IREDUCE_INTRA_ALGORITHM=${algo_name} "
                env+="env=MPIR_CVAR_IREDUCE_TREE_TYPE=${tree_type} "
                env+="env=MPIR_CVAR_IREDUCE_TREE_KVAL=${kval} env=MPIR_CVAR_IREDUCE_TREE_PIPELINE_CHUNK_SIZE=4096 "

                coll_algo_tests+="reduce 5 ${env}${nl}"
                coll_algo_tests+="reduce 10 ${env}${nl}"
                coll_algo_tests+="red3 10 ${env}${nl}"
                coll_algo_tests+="red4 10 ${env}${nl}"
            done
        done
    else #ring algorithm
        #set the environment
        env="${testing_env} env=MPIR_CVAR_IREDUCE_INTRA_ALGORITHM=${algo_name} "
        env+="env=MPIR_CVAR_IREDUCE_RING_CHUNK_SIZE=4096 "

        coll_algo_tests+="reduce 5 ${env}${nl}"
        coll_algo_tests+="reduce 10 ${env}${nl}"
        coll_algo_tests+="red3 10 ${env}${nl}"
        coll_algo_tests+="red4 10 ${env}${nl}"
    fi
done
######### Add tests for Allreduce algorithms ###########

#disable device collectives for allreduce to test MPIR algorithms
testing_env="env=MPIR_CVAR_ALLREDUCE_DEVICE_COLLECTIVE=0 "

#test nb algorithms
testing_env+="env=MPIR_CVAR_ALLREDUCE_INTRA_ALGORITHM=nb "
testing_env+="env=MPIR_CVAR_IALLREDUCE_DEVICE_COLLECTIVE=0 "
algo_names="recexch_single_buffer recexch_multiple_buffer tree_kary tree_knomial"
kvalues="2 3 4"

for algo_name in ${algo_names}; do
    for kval in ${kvalues}; do
        #set the environment
        env="${testing_env} env=MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM=${algo_name} "
        env+="env=MPIR_CVAR_IALLREDUCE_RECEXCH_KVAL=${kval} env=MPIR_CVAR_IALLREDUCE_TREE_PIPELINE_CHUNK_SIZE=4096 "

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
algo_names="recexch_distance_doubling recexch_distance_halving gentran_brucks"
kvalues="2 3 4"

for algo_name in ${algo_names}; do
    for kval in ${kvalues}; do
        #set the environment
        env="${testing_env} env=MPIR_CVAR_IALLGATHER_INTRA_ALGORITHM=${algo_name} "
        env+="env=MPIR_CVAR_IALLGATHER_RECEXCH_KVAL=${kval} "
        env+="env=MPIR_CVAR_IALLGATHER_BRUCKS_KVAL=${kval}"

        coll_algo_tests+="allgather2 10 ${env}${nl}"
        coll_algo_tests+="allgather3 10 ${env}${nl}"
    done
done

######### Add tests for Allgatherv algorithms ###########

#disable device collectives for allgatherv to test MPIR algorithms
testing_env="env=MPIR_CVAR_ALLGATHERV_DEVICE_COLLECTIVE=0 "

#test nb algorithms
testing_env+="env=MPIR_CVAR_ALLGATHERV_INTRA_ALGORITHM=nb "
testing_env+="env=MPIR_CVAR_IALLGATHERV_DEVICE_COLLECTIVE=0 "
algo_names="recexch_distance_doubling recexch_distance_halving gentran_ring"
kvalues="2 3 4"

for algo_name in ${algo_names}; do
    if [ "${algo_name}" != "gentran_ring" ]; then
        for kval in ${kvalues}; do
            env="${testing_env} env=MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM=${algo_name} "
            env+="env=MPIR_CVAR_IALLGATHERV_RECEXCH_KVAL=${kval}"

            coll_algo_tests+="allgatherv2 10 ${env}${nl}"
            coll_algo_tests+="allgatherv3 10 ${env}${nl}"
            coll_algo_tests+="allgatherv4 4 timeLimit=600 ${env}${nl}"
        done
    else
        env="${testing_env} env=MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM=${algo_name} "
        coll_algo_tests+="allgatherv2 10 ${env}${nl}"
        coll_algo_tests+="allgatherv3 10 ${env}${nl}"
        coll_algo_tests+="allgatherv4 4 timeLimit=600 ${env}${nl}"
    fi
done

######### Add tests for Reduce_scatter_block algorithms ###########

#disable device collectives for reduce_scatter_block to test MPIR algorithms
testing_env="env=MPIR_CVAR_REDUCE_SCATTER_BLOCK_DEVICE_COLLECTIVE=0 "

#test nb algorithms
testing_env+="env=MPIR_CVAR_REDUCE_SCATTER_BLOCK_INTRA_ALGORITHM=nb "
testing_env+="env=MPIR_CVAR_IREDUCE_SCATTER_BLOCK_DEVICE_COLLECTIVE=0 "
algo_names="recexch"
kvalues="2 3 4"

for algo_name in ${algo_names}; do
    for kval in ${kvalues}; do
        #set the environment
        env="${testing_env} env=MPIR_CVAR_IREDUCE_SCATTER_BLOCK_INTRA_ALGORITHM=${algo_name} "
        env+="env=MPIR_CVAR_IREDUCE_SCATTER_BLOCK_RECEXCH_KVAL=${kval}"

        coll_algo_tests+="red_scat_block 4 mpiversion=2.2 ${env}${nl}"
        coll_algo_tests+="red_scat_block 5 mpiversion=2.2 ${env}${nl}"
        coll_algo_tests+="red_scat_block 8 mpiversion=2.2 ${env}${nl}"
        coll_algo_tests+="red_scat_block2 4 mpiversion=2.2 ${env}${nl}"
        coll_algo_tests+="red_scat_block2 5 mpiversion=2.2 ${env}${nl}"
        coll_algo_tests+="red_scat_block2 10 mpiversion=2.2 ${env}${nl}"
        coll_algo_tests+="redscatblk3 8 mpiversion=2.2 ${env}${nl}"
        coll_algo_tests+="redscatblk3 10 mpiversion=2.2 ${env}${nl}"
    done
done

######### Add tests for Reduce_scatter algorithms ###########

#disable device collectives for reduce_scatter to test MPIR algorithms
testing_env="env=MPIR_CVAR_REDUCE_SCATTER_DEVICE_COLLECTIVE=0 "

#test nb algorithms
testing_env+="env=MPIR_CVAR_REDUCE_SCATTER_INTRA_ALGORITHM=nb "
testing_env+="env=MPIR_CVAR_IREDUCE_SCATTER_DEVICE_COLLECTIVE=0 "
algo_names="recexch"
kvalues="2 3 4"

for algo_name in ${algo_names}; do
    for kval in ${kvalues}; do
        #set the environment
        env="${testing_env} env=MPIR_CVAR_IREDUCE_SCATTER_INTRA_ALGORITHM=${algo_name} "
        env+="env=MPIR_CVAR_IREDUCE_SCATTER_RECEXCH_KVAL=${kval}"

        coll_algo_tests+="redscat 4 ${env}${nl}"
        coll_algo_tests+="redscat 6 ${env}${nl}"
        coll_algo_tests+="redscat2 4 ${env}${nl}"
        coll_algo_tests+="redscat2 5 ${env}${nl}"
        coll_algo_tests+="redscat2 10 ${env}${nl}"
        coll_algo_tests+="redscat3 8 ${env}${nl}"
    done
done

######### Add tests for Scatter algorithms ###########

#disable device collectives for scatter to test MPIR algorithms
testing_env="env=MPIR_CVAR_SCATTER_DEVICE_COLLECTIVE=0 "

#test nb algorithms
testing_env+="env=MPIR_CVAR_SCATTER_INTRA_ALGORITHM=nb "
testing_env+="env=MPIR_CVAR_ISCATTER_DEVICE_COLLECTIVE=0 "
algo_names="tree"
kvalues="2 3 4"

for algo_name in ${algo_names}; do
    for kval in ${kvalues}; do
        #set the environment
        env="${testing_env} env=MPIR_CVAR_ISCATTER_INTRA_ALGORITHM=${algo_name} "
        env+="env=MPIR_CVAR_ISCATTER_TREE_KVAL=${kval}"

        coll_algo_tests+="scattern 4 ${env}${nl}"
        coll_algo_tests+="scatter2 4 ${env}${nl}"
        coll_algo_tests+="scatter3 4 ${env}${nl}"
    done
done

######### Add tests for Gather algorithms ###########

#disable device collectives for gather to test MPIR algorithms
testing_env="env=MPIR_CVAR_GATHER_DEVICE_COLLECTIVE=0 "

#test nb algorithms
testing_env+="env=MPIR_CVAR_GATHER_INTRA_ALGORITHM=nb "
testing_env+="env=MPIR_CVAR_IGATHER_DEVICE_COLLECTIVE=0 "
algo_names="tree"
kvalues="2 3 4"

for algo_name in ${algo_names}; do
    for kval in ${kvalues}; do
        #set the environment
        env="${testing_env} env=MPIR_CVAR_IGATHER_INTRA_ALGORITHM=${algo_name} "
        env+="env=MPIR_CVAR_IGATHER_TREE_KVAL=${kval}"

        coll_algo_tests+="gather 4 ${env}${nl}"
        coll_algo_tests+="gather2 4 ${env}${nl}"
    done
done

export coll_algo_tests
