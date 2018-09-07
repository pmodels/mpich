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
algo_names="gentran_tree gentran_scatter_recexch_allgather gentran_ring"
tree_types="kary knomial_1 knomial_2"
kvalues="3"

for algo_name in ${algo_names}; do
    for kval in ${kvalues}; do
        if [ ${algo_name} = "gentran_tree" ]; then
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
algo_names="gentran_tree gentran_ring"
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
algo_names="gentran_recexch_single_buffer gentran_recexch_multiple_buffer tree"
tree_types="kary knomial_1 knomial_2"
kvalues="2 3 4"

for algo_name in ${algo_names}; do
    for kval in ${kvalues}; do
        if [ "${algo_name}" = "tree" ]; then
            for tree_type in ${tree_types}; do
                #set the environment
                env="${testing_env} env=MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM=${algo_name} "
                env+="env=MPIR_CVAR_IALLREDUCE_TREE_TYPE=${tree_type} env=MPIR_CVAR_IALLREDUCE_TREE_PIPELINE_CHUNK_SIZE=4096 "
                env+="env=MPIR_CVAR_IALLREDUCE_TREE_KVAL=${kval} "

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
        else
            #set the environment
            env="${testing_env} env=MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM=${algo_name} "
            env+="env=MPIR_CVAR_IALLREDUCE_RECEXCH_KVAL=${kval} "

            coll_algo_tests+="allred 4 arg=100 ${env}${nl}"
            coll_algo_tests+="allred 7 ${env}${nl}"
            coll_algo_tests+="allredmany 4 ${env}${nl}"
            coll_algo_tests+="allred2 4 ${env}${nl}"
            coll_algo_tests+="allred3 10 ${env}${nl}"
            coll_algo_tests+="allred4 4 ${env}${nl}"
            coll_algo_tests+="allred5 5 ${env}${nl}"
            coll_algo_tests+="allred6 4 ${env}${nl}"
            coll_algo_tests+="allred6 7 ${env}${nl}"
        fi
    done
done

######### Add tests for Allgather algorithms ###########

#disable device collectives for allgather to test MPIR algorithms
testing_env="env=MPIR_CVAR_ALLGATHER_DEVICE_COLLECTIVE=0 "

#test nb algorithms
testing_env+="env=MPIR_CVAR_ALLGATHER_INTRA_ALGORITHM=nb "
testing_env+="env=MPIR_CVAR_IALLGATHER_DEVICE_COLLECTIVE=0 "
algo_names="gentran_recexch_doubling gentran_recexch_halving gentran_brucks gentran_ring"
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
algo_names="gentran_recexch_doubling gentran_recexch_halving gentran_ring gentran_brucks"
kvalues="2 3 4"

for algo_name in ${algo_names}; do
    if [ "${algo_name}" != "gentran_ring" ]; then
        for kval in ${kvalues}; do
            env="${testing_env} env=MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM=${algo_name} "
            if [ "${algo_name}" != "gentran_brucks" ]; then
                env+="env=MPIR_CVAR_IALLGATHERV_BRUCKS_KVAL=${kval}"
            else
                env+="env=MPIR_CVAR_IALLGATHERV_RECEXCH_KVAL=${kval}"
            fi

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
algo_names="gentran_recexch"
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
algo_names="gentran_recexch"
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
algo_names="gentran_tree"
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
algo_names="gentran_tree"
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

######### Add tests for Alltoall algorithms ###########

#disable device collectives for allgather to test MPIR algorithms
testing_env="env=MPIR_CVAR_ALLTOALL_DEVICE_COLLECTIVE=0 "

#test nb algorithms
testing_env+="env=MPIR_CVAR_ALLTOALL_INTRA_ALGORITHM=nb "
testing_env+="env=MPIR_CVAR_IALLTOALL_DEVICE_COLLECTIVE=0 "
algo_names="gentran_ring"

for algo_name in ${algo_names}; do
    env="${testing_env} env=MPIR_CVAR_IALLTOALL_INTRA_ALGORITHM=${algo_name} "

    coll_algo_tests+="alltoall1 8 ${env}${nl}"
done

algo_names="gentran_brucks"
kvalues="2 3 4"

for algo_name in ${algo_names}; do
    for kval in ${kvalues}; do
        env="${testing_env} env=MPIR_CVAR_IALLTOALL_INTRA_ALGORITHM=${algo_name} "
        env+="env=MPIR_CVAR_IALLTOALL_BRUCKS_KVAL=${kval} "

        coll_algo_tests+="alltoall1 8 ${env} env=MPIR_CVAR_IALLTOALL_BRUCKS_BUFFER_PER_NBR=0${nl}"
        coll_algo_tests+="alltoall1 8 ${env} env=MPIR_CVAR_IALLTOALL_BRUCKS_BUFFER_PER_NBR=1${nl}"
    done
done

########## Add tests for scan algorithms ############

#disable device collectives for scan to test MPIR algorithms
testing_env="env=MPIR_CVAR_SCAN_DEVICE_COLLECTIVE=0 "

#test nb algorithms
testing_env+="env=MPIR_CVAR_SCAN_INTRA_ALGORITHM=nb "
testing_env+="env=MPIR_CVAR_ISCAN_DEVICE_COLLECTIVE=0 "
algo_names="gentran_recursive_doubling"

for algo_name in ${algo_names}; do
    #set the environment
    env="${testing_env} env=MPIR_CVAR_ISCAN_INTRA_ALGORITHM=${algo_name} "

    coll_algo_tests+="scantst 4"
done

######### Add tests for ineighbor_alltoallw algorithms ###########

#disable device collectives for neighbor_alltoallw to test mpir algorithms
testing_env="env=MPIR_CVAR_NEIGHBOR_ALLTOALLW_DEVICE_COLLECTIVE=0 "

#test nb algorithms
testing_env+="env=MPIR_CVAR_NEIGHBOR_ALLTOALLW_INTRA_ALGORITHM=nb "
testing_env+="env=MPIR_CVAR_INEIGHBOR_ALLTOALLW_DEVICE_COLLECTIVE=0 "
algo_names="gentran_linear"

for algo_name in ${algo_names}; do
    #set the environment
    env="${testing_env} env=MPIR_CVAR_INEIGHBOR_ALLTOALLW_INTRA_ALGORITHM=${algo_name} "

    coll_algo_tests+="neighb_alltoallw 4 mpiversion=3.0 ${env}${nl}"
done

######### Add tests for Alltoallv algorithms ###########

#disable device collectives for alltoallv to test MPIR algorithms
testing_env="env=MPIR_CVAR_ALLTOALLV_DEVICE_COLLECTIVE=0 "

#test nb algorithms
testing_env+="env=MPIR_CVAR_ALLTOALLV_INTRA_ALGORITHM=nb "
testing_env+="env=MPIR_CVAR_IALLTOALLV_DEVICE_COLLECTIVE=0 "
algo_names="gentran_scattered"
batchsizes="1 2 4"
outstandingtasks="4 8"
for algo_name in ${algo_names}; do
    env="${testing_env} env=MPIR_CVAR_IALLTOALLV_INTRA_ALGORITHM=${algo_name} "
    if [ ${algo_name} = "gentran_scattered" ]; then
        for task in ${outstandingtasks}; do
            for batchsize in ${batchsizes}; do
                env="${testing_env} env=MPIR_CVAR_IALLTOALLV_INTRA_ALGORITHM=${algo_name} "
                env+="env=MPIR_CVAR_IALLTOALLV_SCATTERED_BATCH_SIZE=${batchsize} "
                env+="env=MPIR_CVAR_IALLTOALLV_SCATTERED_OUTSTANDING_TASKS=${task} "
                 coll_algo_tests+="alltoallv 8 ${env}${nl}"
                 coll_algo_tests+="alltoallv0 10 ${env}${nl}"
            done
        done
    fi
done

######### Add tests for Ineighbor_allgather algorithms ###########

#disable device collectives for neighbor_allgather to test MPIR algorithms
testing_env="env=MPIR_CVAR_NEIGHBOR_ALLGATHER_DEVICE_COLLECTIVE=0 "

#test nb algorithms
testing_env+="env=MPIR_CVAR_NEIGHBOR_ALLGATHER_INTRA_ALGORITHM=nb "
testing_env+="env=MPIR_CVAR_INEIGHBOR_ALLGATHER_DEVICE_COLLECTIVE=0 "
algo_names="gentran_linear"

for algo_name in ${algo_names}; do
    #set the environment
    env="${testing_env} env=MPIR_CVAR_INEIGHBOR_ALLGATHER_INTRA_ALGORITHM=${algo_name} "

    coll_algo_tests+="neighb_allgather 4 mpiversion=3.0 ${env}${nl}"
done

######### Add tests for Ineighbor_allgatherv algorithms ###########

#disable device collectives for neighbor_allgatherv to test MPIR algorithms
testing_env="env=MPIR_CVAR_NEIGHBOR_ALLGATHERV_DEVICE_COLLECTIVE=0 "

#test nb algorithms
testing_env+="env=MPIR_CVAR_NEIGHBOR_ALLGATHERV_INTRA_ALGORITHM=nb "
testing_env+="env=MPIR_CVAR_INEIGHBOR_ALLGATHERV_DEVICE_COLLECTIVE=0 "
algo_names="gentran_linear"

for algo_name in ${algo_names}; do
    #set the environment
    env="${testing_env} env=MPIR_CVAR_INEIGHBOR_ALLGATHERV_INTRA_ALGORITHM=${algo_name} "

    coll_algo_tests+="neighb_allgatherv 4 mpiversion=3.0 ${env}${nl}"
done

######### Add tests for Ineighbor_alltoall algorithms ###########

#disable device collectives for neighbor_alltoall to test MPIR algorithms
testing_env="env=MPIR_CVAR_NEIGHBOR_ALLTOALL_DEVICE_COLLECTIVE=0 "

#test nb algorithms
testing_env+="env=MPIR_CVAR_NEIGHBOR_ALLTOALL_INTRA_ALGORITHM=nb "
testing_env+="env=MPIR_CVAR_INEIGHBOR_ALLTOALL_DEVICE_COLLECTIVE=0 "
algo_names="gentran_linear"

for algo_name in ${algo_names}; do
    #set the environment
    env="${testing_env} env=MPIR_CVAR_INEIGHBOR_ALLTOALL_INTRA_ALGORITHM=${algo_name} "

    coll_algo_tests+="neighb_alltoall 4 mpiversion=3.0 ${env}${nl}"
done

######### Add tests for Ineighbor_alltoallv algorithms ###########

#disable device collectives for neighbor_alltoallv to test MPIR algorithms
testing_env="env=MPIR_CVAR_NEIGHBOR_ALLTOALLV_DEVICE_COLLECTIVE=0 "

#test nb algorithms
testing_env+="env=MPIR_CVAR_NEIGHBOR_ALLTOALLV_INTRA_ALGORITHM=nb "
testing_env+="env=MPIR_CVAR_INEIGHBOR_ALLTOALLV_DEVICE_COLLECTIVE=0 "
algo_names="gentran_linear"

for algo_name in ${algo_names}; do
    #set the environment
    env="${testing_env} env=MPIR_CVAR_INEIGHBOR_ALLTOALLV_INTRA_ALGORITHM=${algo_name} "

    coll_algo_tests+="neighb_alltoallv 4 mpiversion=3.0 ${env}${nl}"
done

########## Add tests for intra-node bcast algorithms ############

#use release gather based intra-node bcast
testing_env="env=MPIR_CVAR_BCAST_POSIX_INTRA_ALGORITHM=release_gather "

testing_env+="env=MPIR_CVAR_COLL_SHM_LIMIT_PER_NODE=131072 " #128MB
buffer_sizes="16384 32768"
num_cells="2 4"
tree_types="knomial_1 knomial_2"
kvalues="8 64"

for buf_size in ${buffer_sizes}; do
    for num_cell in ${num_cells}; do
        for kval in ${kvalues}; do
            for tree_type in ${tree_types}; do
                #set the environment
                env="${testing_env} env=MPIR_CVAR_BCAST_INTRANODE_BUFFER_TOTAL_SIZE=${buf_size} "
                env+="env=MPIR_CVAR_BCAST_INTRANODE_NUM_CELLS=${num_cell} "
                env+="env=MPIR_CVAR_BCAST_INTRANODE_TREE_KVAL=${kval} "
                env+="env=MPIR_CVAR_BCAST_INTRANODE_TREE_TYPE=${tree_type} "

                coll_algo_tests+="bcasttest 10 ${env}${nl}"
                coll_algo_tests+="bcastzerotype 5 ${env}${nl}"
            done
        done
    done
done

########## Add tests for intra-node reduce algorithms ############

#use release gather based intra-node reduce
testing_env="env=MPIR_CVAR_REDUCE_POSIX_INTRA_ALGORITHM=release_gather "

testing_env+="env=MPIR_CVAR_COLL_SHM_LIMIT_PER_NODE=131072 " #128MB
buffer_sizes="16384 32768"
num_cells="2 4"
tree_types="knomial_1 knomial_2"
kvalues="4 8"

for buf_size in ${buffer_sizes}; do
    for num_cell in ${num_cells}; do
        for kval in ${kvalues}; do
            for tree_type in ${tree_types}; do
                #set the environment
                env="${testing_env} env=MPIR_CVAR_REDUCE_INTRANODE_BUFFER_TOTAL_SIZE=${buf_size} "
                env+="env=MPIR_CVAR_REDUCE_INTRANODE_NUM_CELLS=${num_cell} "
                env+="env=MPIR_CVAR_REDUCE_INTRANODE_TREE_KVAL=${kval} "
                env+="env=MPIR_CVAR_REDUCE_INTRANODE_TREE_TYPE=${tree_type} "

                coll_algo_tests+="reduce 5 ${env}${nl}"
                coll_algo_tests+="reduce 10 ${env}${nl}"
                coll_algo_tests+="red3 10 ${env}${nl}"
                coll_algo_tests+="red4 10 ${env}${nl}"
            done
        done
    done
done

export coll_algo_tests
