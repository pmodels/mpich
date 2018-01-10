#! /bin/bash
##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

testlist_gpu=coll/testlist.gpu
testlist_cvar=coll/testlist.cvar
#start an empty testlist
echo "" > ${testlist_cvar}

######### Global environment variables that apply to all tests

#silently fallback to other algorithms, if the selected algorithm is
#not suitable
global_env="${global_env} env=MPIR_CVAR_COLLECTIVE_FALLBACK=silent"


########## Add tests for bcast algorithms ############

#start with the global environment variables
testing_env=${global_env}

#disable device collectives for bcast to test MPIR algorithms
testing_env="${testing_env} env=MPIR_CVAR_BCAST_DEVICE_COLLECTIVE=0"

#test nb algorithms
testing_env="${testing_env} env=MPIR_CVAR_BCAST_INTRA_ALGORITHM=nb"
testing_env="${testing_env} env=MPIR_CVAR_IBCAST_DEVICE_COLLECTIVE=0"
algo_names="gentran_tree gentran_scatterv_recexch_allgatherv gentran_ring"
tree_types="kary knomial_1 knomial_2"
kvalues="3"

for algo_name in ${algo_names}; do
    for kval in ${kvalues}; do
        if [ ${algo_name} = "gentran_tree" ]; then
            for tree_type in ${tree_types}; do
                #set the environment
                env="${testing_env} env=MPIR_CVAR_IBCAST_INTRA_ALGORITHM=${algo_name}"
                env="${env} env=MPIR_CVAR_IBCAST_TREE_TYPE=${tree_type}"
                env="${env} env=MPIR_CVAR_IBCAST_TREE_KVAL=${kval} env=MPIR_CVAR_IBCAST_TREE_PIPELINE_CHUNK_SIZE=4096"
                env="${env} env=MPIR_CVAR_IBCAST_RING_CHUNK_SIZE=4096"

                echo "bcasttest 10 ${env}" >> ${testlist_cvar}
                echo "bcastzerotype 5 ${env}" >> ${testlist_cvar}
                echo "p_bcast 4 ${env}" >> ${testlist_cvar}
                echo "p_bcast2 8 ${env}" >> ${testlist_cvar}
                env=""
            done
        else
            #set the environment
            env="${testing_env} env=MPIR_CVAR_IBCAST_INTRA_ALGORITHM=${algo_name}"
            env="${env} env=MPIR_CVAR_IBCAST_SCATTER_KVAL=${kval} env=MPIR_CVAR_IBCAST_ALLGATHER_RECEXCH_KVAL=${kval}"

            echo "bcasttest 10 ${env}" >> ${testlist_cvar}
            echo "bcastzerotype 5 ${env}" >> ${testlist_cvar}
            echo "p_bcast 4 ${env}" >> ${testlist_cvar}
            echo "p_bcast2 8 ${env}" >> ${testlist_cvar}
            env=""
        fi
    done
done

########## Add tests for reduce algorithms ############

#start with the global environment variables
testing_env=${global_env}

#disable device collectives for reduce to test MPIR algorithms
testing_env="${testing_env} env=MPIR_CVAR_REDUCE_DEVICE_COLLECTIVE=0"

#test nb algorithms
testing_env="${testing_env} env=MPIR_CVAR_REDUCE_INTRA_ALGORITHM=nb"
testing_env="${testing_env} env=MPIR_CVAR_IREDUCE_DEVICE_COLLECTIVE=0"
algo_names="gentran_tree gentran_ring"
tree_types="kary knomial_1 knomial_2"
kvalues="3"

for algo_name in ${algo_names}; do
    if [ ${algo_name} = "tree" ]; then
        for tree_type in ${tree_types}; do
            for kval in ${kvalues}; do
                #set the environment
                env="${testing_env} env=MPIR_CVAR_IREDUCE_INTRA_ALGORITHM=${algo_name}"
                env="${env} env=MPIR_CVAR_IREDUCE_TREE_TYPE=${tree_type}"
                env="${env} env=MPIR_CVAR_IREDUCE_TREE_KVAL=${kval} env=MPIR_CVAR_IREDUCE_TREE_PIPELINE_CHUNK_SIZE=4096"

                echo "reduce 5 ${env}" >> ${testlist_cvar}
                echo "reduce 5 arg=-evenmemtype=host arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
                echo "reduce 5 arg=-evenmemtype=reg_host arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
                echo "reduce 5 arg=-evenmemtype=device arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
                echo "reduce 10 ${env}" >> ${testlist_cvar}
                echo "reduce 10 arg=-evenmemtype=host arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
                echo "reduce 10 arg=-evenmemtype=reg_host arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
                echo "reduce 10 arg=-evenmemtype=device arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
                echo "red3 10 ${env}" >> ${testlist_cvar}
                echo "red4 10 ${env}" >> ${testlist_cvar}
                env=""
            done
        done
    else #ring algorithm
        #set the environment
        env="${testing_env} env=MPIR_CVAR_IREDUCE_INTRA_ALGORITHM=${algo_name}"
        env="${env} env=MPIR_CVAR_IREDUCE_RING_CHUNK_SIZE=4096"

        echo "reduce 5 ${env}" >> ${testlist_cvar}
        echo "reduce 5 arg=-evenmemtype=host arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
        echo "reduce 5 arg=-evenmemtype=reg_host arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
        echo "reduce 5 arg=-evenmemtype=device arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
        echo "reduce 10 ${env}" >> ${testlist_cvar}
        echo "reduce 10 arg=-evenmemtype=host arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
        echo "reduce 10 arg=-evenmemtype=reg_host arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
        echo "reduce 10 arg=-evenmemtype=device arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
        echo "red3 10 ${env}" >> ${testlist_cvar}
        echo "red4 10 ${env}" >> ${testlist_cvar}
        env=""
    fi
done

######### Add tests for Allreduce algorithms ###########

#start with the global environment variables
testing_env=${global_env}

#disable device collectives for allreduce to test MPIR algorithms
testing_env="${testing_env} env=MPIR_CVAR_ALLREDUCE_DEVICE_COLLECTIVE=0"

#test nb algorithms
testing_env="${testing_env} env=MPIR_CVAR_ALLREDUCE_INTRA_ALGORITHM=nb"
testing_env="${testing_env} env=MPIR_CVAR_IALLREDUCE_DEVICE_COLLECTIVE=0"
algo_names="gentran_recexch_single_buffer gentran_recexch_multiple_buffer gentran_tree gentran_ring gentran_recexch_reduce_scatter_recexch_allgatherv"
tree_types="kary knomial_1 knomial_2"
kvalues="2 3 4"

for algo_name in ${algo_names}; do
    if [ "${algo_name}" != "gentran_ring" ]; then
        for kval in ${kvalues}; do
            if [ "${algo_name}" = "gentran_tree" ]; then
                for tree_type in ${tree_types}; do
                    #set the environment
                    env="${testing_env} env=MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM=${algo_name}"
                    env="${env} env=MPIR_CVAR_IALLREDUCE_TREE_TYPE=${tree_type} env=MPIR_CVAR_IALLREDUCE_TREE_PIPELINE_CHUNK_SIZE=4096"
                    env="${env} env=MPIR_CVAR_IALLREDUCE_TREE_KVAL=${kval}"

                    echo "allred 4 arg=-count=100 ${env}" >> ${testlist_cvar}
                    echo "allred 4 arg=-count=100 arg=-evenmemtype=host arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
                    echo "allred 4 arg=-count=100 arg=-evenmemtype=reg_host arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
                    echo "allred 4 arg=-count=100 arg=-evenmemtype=device arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
                    echo "allred 7 arg=-count=10 ${env}" >> ${testlist_cvar}
                    echo "allred 7 arg=-count=10 arg=-evenmemtype=host arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
                    echo "allred 7 arg=-count=10 arg=-evenmemtype=reg_host arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
                    echo "allred 7 arg=-count=10 arg=-evenmemtype=device arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
                    echo "allredmany 4 ${env}" >> ${testlist_cvar}
                    echo "allred2 4 ${env}" >> ${testlist_cvar}
                    echo "allred2 4 arg=-evenmemtype=host arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
                    echo "allred2 4 arg=-evenmemtype=reg_host arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
                    echo "allred2 4 arg=-evenmemtype=device arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
                    echo "allred3 10 ${env}" >> ${testlist_cvar}
                    echo "allred4 4 ${env}" >> ${testlist_cvar}
                    echo "allred5 5 ${env}" >> ${testlist_cvar}
                    echo "allred6 4 ${env}" >> ${testlist_cvar}
                    echo "allred6 7 ${env}" >> ${testlist_cvar}
                    env=""
                done
            else #test recursive exchange algorithms
                #set the environment
                env="${testing_env} env=MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM=${algo_name}"
                env="${env} env=MPIR_CVAR_IALLREDUCE_RECEXCH_KVAL=${kval}"

                echo "allred 4 arg=-count=100 ${env}" >> ${testlist_cvar}
                echo "allred 4 arg=-count=100 arg=-evenmemtype=host arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
                echo "allred 4 arg=-count=100 arg=-evenmemtype=reg_host arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
                echo "allred 4 arg=-count=100 arg=-evenmemtype=device arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
                echo "allred 7 arg=-count=10 ${env}" >> ${testlist_cvar}
                echo "allred 7 arg=-count=10 arg=-evenmemtype=host arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
                echo "allred 7 arg=-count=10 arg=-evenmemtype=reg_host arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
                echo "allred 7 arg=-count=10 arg=-evenmemtype=device arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
                echo "allredmany 4 ${env}" >> ${testlist_cvar}
                echo "allred2 4 ${env}" >> ${testlist_cvar}
                echo "allred2 4 arg=-evenmemtype=host arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
                echo "allred2 4 arg=-evenmemtype=reg_host arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
                echo "allred2 4 arg=-evenmemtype=device arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
                echo "allred3 10 ${env}" >> ${testlist_cvar}
                echo "allred4 4 ${env}" >> ${testlist_cvar}
                echo "allred5 5 ${env}" >> ${testlist_cvar}
                echo "allred6 4 ${env}" >> ${testlist_cvar}
                echo "allred6 7 ${env}" >> ${testlist_cvar}
                env=""
            fi
        done
    else #test ring algorithm
        #set the environment
        env="${testing_env} env=MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM=${algo_name}"

        echo "allred 4 arg=-count=100 ${env}" >> ${testlist_cvar}
        echo "allred 4 arg=-count=100 arg=-evenmemtype=host arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
        echo "allred 4 arg=-count=100 arg=-evenmemtype=reg_host arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
        echo "allred 4 arg=-count=100 arg=-evenmemtype=device arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
        echo "allred 7 arg=-count=10 ${env}" >> ${testlist_cvar}
        echo "allred 7 arg=-count=10 arg=-evenmemtype=host arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
        echo "allred 7 arg=-count=10 arg=-evenmemtype=reg_host arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
        echo "allred 7 arg=-count=10 arg=-evenmemtype=device arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
        echo "allredmany 4 ${env}" >> ${testlist_cvar}
        echo "allred2 4 ${env}" >> ${testlist_cvar}
        echo "allred2 4 arg=-evenmemtype=host arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
        echo "allred2 4 arg=-evenmemtype=reg_host arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
        echo "allred2 4 arg=-evenmemtype=device arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
        echo "allred3 10 ${env}" >> ${testlist_cvar}
        echo "allred4 4 ${env}" >> ${testlist_cvar}
        echo "allred5 5 ${env}" >> ${testlist_cvar}
        echo "allred6 4 ${env}" >> ${testlist_cvar}
        echo "allred6 7 ${env}" >> ${testlist_cvar}
        env=""
    fi
done

######### Add tests for Allgather algorithms ###########

#start with the global environment variables
testing_env=${global_env}

#disable device collectives for allgather to test MPIR algorithms
testing_env="${testing_env} env=MPIR_CVAR_ALLGATHER_DEVICE_COLLECTIVE=0"

#test nb algorithms
testing_env="${testing_env} env=MPIR_CVAR_ALLGATHER_INTRA_ALGORITHM=nb"
testing_env="${testing_env} env=MPIR_CVAR_IALLGATHER_DEVICE_COLLECTIVE=0"
algo_names="gentran_recexch_doubling gentran_recexch_halving gentran_brucks gentran_ring"
kvalues="2 3 4"

for algo_name in ${algo_names}; do
    for kval in ${kvalues}; do
        #set the environment
        env="${testing_env} env=MPIR_CVAR_IALLGATHER_INTRA_ALGORITHM=${algo_name}"
        env="${env} env=MPIR_CVAR_IALLGATHER_RECEXCH_KVAL=${kval}"
        env="${env} env=MPIR_CVAR_IALLGATHER_BRUCKS_KVAL=${kval}"

        echo "allgather2 10 ${env}" >> ${testlist_cvar}
        echo "allgather3 10 ${env}" >> ${testlist_cvar}
        env=""
    done
done

######### Add tests for Allgatherv algorithms ###########

#start with the global environment variables
testing_env=${global_env}

#disable device collectives for allgatherv to test MPIR algorithms
testing_env="${testing_env} env=MPIR_CVAR_ALLGATHERV_DEVICE_COLLECTIVE=0"

#test nb algorithms
testing_env="${testing_env} env=MPIR_CVAR_ALLGATHERV_INTRA_ALGORITHM=nb"
testing_env="${testing_env} env=MPIR_CVAR_IALLGATHERV_DEVICE_COLLECTIVE=0"
algo_names="gentran_recexch_doubling gentran_recexch_halving gentran_ring gentran_brucks"
kvalues="2 3 4"

for algo_name in ${algo_names}; do
    if [ "${algo_name}" != "gentran_ring" ]; then
        for kval in ${kvalues}; do
            env="${testing_env} env=MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM=${algo_name}"
            if [ "${algo_name}" = "gentran_brucks" ]; then
                env="${env} env=MPIR_CVAR_IALLGATHERV_BRUCKS_KVAL=${kval}"
            else
                env="${env} env=MPIR_CVAR_IALLGATHERV_RECEXCH_KVAL=${kval}"
            fi

            echo "allgatherv2 10 ${env}" >> ${testlist_cvar}
            echo "allgatherv3 10 ${env}" >> ${testlist_cvar}
            echo "allgatherv4 4 timeLimit=600 ${env}" >> ${testlist_cvar}
            env=""
        done
    else
        env="${testing_env} env=MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM=${algo_name}"
        echo "allgatherv2 10 ${env}" >> ${testlist_cvar}
        echo "allgatherv3 10 ${env}" >> ${testlist_cvar}
        echo "allgatherv4 4 timeLimit=600 ${env}" >> ${testlist_cvar}
        env=""
    fi
done

######### Add tests for Reduce_scatter_block algorithms ###########

#start with the global environment variables
testing_env=${global_env}

#disable device collectives for reduce_scatter_block to test MPIR algorithms
testing_env="${testing_env} env=MPIR_CVAR_REDUCE_SCATTER_BLOCK_DEVICE_COLLECTIVE=0"

#test nb algorithms
testing_env="${testing_env} env=MPIR_CVAR_REDUCE_SCATTER_BLOCK_INTRA_ALGORITHM=nb"
testing_env="${testing_env} env=MPIR_CVAR_IREDUCE_SCATTER_BLOCK_DEVICE_COLLECTIVE=0"
algo_names="gentran_recexch"
kvalues="2 3 4"

for algo_name in ${algo_names}; do
    for kval in ${kvalues}; do
        #set the environment
        env="${testing_env} env=MPIR_CVAR_IREDUCE_SCATTER_BLOCK_INTRA_ALGORITHM=${algo_name}"
        env="${env} env=MPIR_CVAR_IREDUCE_SCATTER_BLOCK_RECEXCH_KVAL=${kval}"

        echo "red_scat_block 4 ${env}" >> ${testlist_cvar}
        echo "red_scat_block 5 ${env}" >> ${testlist_cvar}
        echo "red_scat_block 8 ${env}" >> ${testlist_cvar}
        echo "red_scat_block2 4 ${env}" >> ${testlist_cvar}
        echo "red_scat_block2 5 ${env}" >> ${testlist_cvar}
        echo "red_scat_block2 10 ${env}" >> ${testlist_cvar}
        echo "redscatblk3 8 ${env}" >> ${testlist_cvar}
        echo "redscatblk3 10 ${env}" >> ${testlist_cvar}
        env=""
    done
done


######### Add tests for Reduce_scatter algorithms ###########

#start with the global environment variables
testing_env=${global_env}

#disable device collectives for reduce_scatter to test MPIR algorithms
testing_env="${testing_env} env=MPIR_CVAR_REDUCE_SCATTER_DEVICE_COLLECTIVE=0"

#test nb algorithms
testing_env="${testing_env} env=MPIR_CVAR_REDUCE_SCATTER_INTRA_ALGORITHM=nb"
testing_env="${testing_env} env=MPIR_CVAR_IREDUCE_SCATTER_DEVICE_COLLECTIVE=0"
algo_names="gentran_recexch"
kvalues="2 3 4"

for algo_name in ${algo_names}; do
    for kval in ${kvalues}; do
        #set the environment
        env="${testing_env} env=MPIR_CVAR_IREDUCE_SCATTER_INTRA_ALGORITHM=${algo_name}"
        env="${env} env=MPIR_CVAR_IREDUCE_SCATTER_RECEXCH_KVAL=${kval}"

        echo "redscat 4 ${env}" >> ${testlist_cvar}
        echo "redscat 6 ${env}" >> ${testlist_cvar}
        echo "redscat2 4 ${env}" >> ${testlist_cvar}
        echo "redscat2 5 ${env}" >> ${testlist_cvar}
        echo "redscat2 10 ${env}" >> ${testlist_cvar}
        echo "redscat3 8 ${env}" >> ${testlist_cvar}
        env=""
    done
done

######### Add tests for Scatter algorithms ###########

#start with the global environment variables
testing_env=${global_env}

#disable device collectives for scatter to test MPIR algorithms
testing_env="${testing_env} env=MPIR_CVAR_SCATTER_DEVICE_COLLECTIVE=0"

#test nb algorithms
testing_env="${testing_env} env=MPIR_CVAR_SCATTER_INTRA_ALGORITHM=nb"
testing_env="${testing_env} env=MPIR_CVAR_ISCATTER_DEVICE_COLLECTIVE=0"
algo_names="gentran_tree"
kvalues="2 3 4"

for algo_name in ${algo_names}; do
    for kval in ${kvalues}; do
        #set the environment
        env="${testing_env} env=MPIR_CVAR_ISCATTER_INTRA_ALGORITHM=${algo_name}"
        env="${env} env=MPIR_CVAR_ISCATTER_TREE_KVAL=${kval}"

        echo "scattern 4 ${env}" >> ${testlist_cvar}
        echo "scatter2 4 ${env}" >> ${testlist_cvar}
        echo "scatter3 4 ${env}" >> ${testlist_cvar}
        env=""
    done
done

######### Add tests for Gather algorithms ###########

#start with the global environment variables
testing_env=${global_env}

#disable device collectives for gather to test MPIR algorithms
testing_env="${testing_env} env=MPIR_CVAR_GATHER_DEVICE_COLLECTIVE=0"

#test nb algorithms
testing_env="${testing_env} env=MPIR_CVAR_GATHER_INTRA_ALGORITHM=nb"
testing_env="${testing_env} env=MPIR_CVAR_IGATHER_DEVICE_COLLECTIVE=0"
algo_names="gentran_tree"
kvalues="2 3 4"

for algo_name in ${algo_names}; do
    for kval in ${kvalues}; do
        #set the environment
        env="${testing_env} env=MPIR_CVAR_IGATHER_INTRA_ALGORITHM=${algo_name}"
        env="${env} env=MPIR_CVAR_IGATHER_TREE_KVAL=${kval}"

        echo "gather 4 ${env}" >> ${testlist_cvar}
        echo "gather2 4 ${env}" >> ${testlist_cvar}
        env=""
    done
done

######### Add tests for Alltoall algorithms ###########

#start with the global environment variables
testing_env=${global_env}

#disable device collectives for alltoall to test MPIR algorithms
testing_env="${testing_env} env=MPIR_CVAR_ALLTOALL_DEVICE_COLLECTIVE=0"

#test nb algorithms
testing_env="${testing_env} env=MPIR_CVAR_ALLTOALL_INTRA_ALGORITHM=nb"
testing_env="${testing_env} env=MPIR_CVAR_IALLTOALL_DEVICE_COLLECTIVE=0"
algo_names="gentran_ring gentran_scattered"
batchsizes="1 2 4"
outstandingtasks="4 8"

for algo_name in ${algo_names}; do
    if [ "${algo_name}" != "gentran_scattered" ]; then
        env="${testing_env} env=MPIR_CVAR_IALLTOALL_INTRA_ALGORITHM=${algo_name}"
        echo "alltoall1 8 ${env}" >> ${testlist_cvar}
        env=""
    else
        for task in ${outstandingtasks}; do
            for batchsize in ${batchsizes}; do
                env="${testing_env} env=MPIR_CVAR_IALLTOALL_INTRA_ALGORITHM=${algo_name}"
                env="${env} env=MPIR_CVAR_IALLTOALL_SCATTERED_BATCH_SIZE=${batchsize}"
                env="${env} env=MPIR_CVAR_IALLTOALL_SCATTERED_OUTSTANDING_TASKS=${task}"

                echo "alltoall1 8 ${env}" >> ${testlist_cvar}
                env=""
            done
        done
    fi

done

algo_names="gentran_brucks"
kvalues="2 3 4"

for algo_name in ${algo_names}; do
    for kval in ${kvalues}; do
        env="${testing_env} env=MPIR_CVAR_IALLTOALL_INTRA_ALGORITHM=${algo_name}"
        env="${env} env=MPIR_CVAR_IALLTOALL_BRUCKS_KVAL=${kval}"

        echo "alltoall1 8 ${env} env=MPIR_CVAR_IALLTOALL_BRUCKS_BUFFER_PER_NBR=0" >> ${testlist_cvar}
        echo "alltoall1 8 ${env} env=MPIR_CVAR_IALLTOALL_BRUCKS_BUFFER_PER_NBR=1" >> ${testlist_cvar}
        env=""
    done
done

########## Add tests for scan algorithms ############

#start with the global environment variables
testing_env=${global_env}

#disable device collectives for scan to test MPIR algorithms
testing_env="${testing_env} env=MPIR_CVAR_SCAN_DEVICE_COLLECTIVE=0"

#test nb algorithms
testing_env="${testing_env} env=MPIR_CVAR_SCAN_INTRA_ALGORITHM=nb"
testing_env="${testing_env} env=MPIR_CVAR_ISCAN_DEVICE_COLLECTIVE=0"
algo_names="gentran_recursive_doubling"

for algo_name in ${algo_names}; do
    #set the environment
    env="${testing_env} env=MPIR_CVAR_ISCAN_INTRA_ALGORITHM=${algo_name}"

    echo "scantst 4 ${env}" >> ${testlist_cvar}
    echo "op_coll 4 ${env}" >> ${testlist_cvar}
    echo "op_coll 4 arg=-evenmemtype=host arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
    echo "op_coll 4 arg=-evenmemtype=reg_host arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
    echo "op_coll 4 arg=-evenmemtype=device arg=-oddmemtype=device ${env}" >> ${testlist_gpu}
    env=""
done

######### Add tests for ineighbor_alltoallw algorithms ###########

#start with the global environment variables
testing_env=${global_env}

#disable device collectives for ineighbor_alltoallw to test mpir algorithms
testing_env="${testing_env} env=MPIR_CVAR_INEIGHBOR_ALLTOALLW_DEVICE_COLLECTIVE=0"

#test nb algorithms
testing_env="${testing_env} env=MPIR_CVAR_NEIGHBOR_ALLTOALLW_INTRA_ALGORITHM=nb"
testing_env="${testing_env} env=MPIR_CVAR_NEIGHBOR_ALLTOALLW_DEVICE_COLLECTIVE=0"
algo_names="gentran_linear"

for algo_name in ${algo_names}; do
    #set the environment
    env="${testing_env} env=MPIR_CVAR_INEIGHBOR_ALLTOALLW_INTRA_ALGORITHM=${algo_name}"

    echo "neighb_alltoallw 4 ${env}" >> ${testlist_cvar}
    env=""
done

######### Add tests for Alltoallv algorithms ###########

#start with the global environment variables
testing_env=${global_env}

#disable device collectives for alltoallv to test MPIR algorithms
testing_env="${testing_env} env=MPIR_CVAR_ALLTOALLV_DEVICE_COLLECTIVE=0"

#test nb algorithms
testing_env="${testing_env} env=MPIR_CVAR_ALLTOALLV_INTRA_ALGORITHM=nb"
testing_env="${testing_env} env=MPIR_CVAR_IALLTOALLV_DEVICE_COLLECTIVE=0"
algo_names="gentran_scattered"
batchsizes="1 2 4"
outstandingtasks="4 8"
for algo_name in ${algo_names}; do
    env="${testing_env} env=MPIR_CVAR_IALLTOALLV_INTRA_ALGORITHM=${algo_name}"
    if [ ${algo_name} = "gentran_scattered" ]; then
        for task in ${outstandingtasks}; do
            for batchsize in ${batchsizes}; do
                env="${testing_env} env=MPIR_CVAR_IALLTOALLV_INTRA_ALGORITHM=${algo_name}"
                env="${env} env=MPIR_CVAR_IALLTOALLV_SCATTERED_BATCH_SIZE=${batchsize}"
                env="${env} env=MPIR_CVAR_IALLTOALLV_SCATTERED_OUTSTANDING_TASKS=${task}"
                echo "alltoallv 8 ${env}" >> ${testlist_cvar}
                echo "alltoallv0 10 ${env}" >> ${testlist_cvar}
                env=""
            done
        done
    fi
done

algo_names="gentran_blocked gentran_inplace"
for algo_name in ${algo_names}; do
    #set the environment
    env="${testing_env} env=MPIR_CVAR_IALLTOALLV_INTRA_ALGORITHM=${algo_name}"

    echo "alltoallv 10 ${env}" >> ${testlist_cvar}
    echo "alltoallv0 10 ${env}" >> ${testlist_cvar}
    env=""
done

######### Add tests for Ineighbor_allgather algorithms ###########

#start with the global environment variables
testing_env=${global_env}

#disable device collectives for ineighbor_allgather to test MPIR algorithms
testing_env="${testing_env} env=MPIR_CVAR_INEIGHBOR_ALLGATHER_DEVICE_COLLECTIVE=0"

#test nb algorithms
testing_env="${testing_env} env=MPIR_CVAR_NEIGHBOR_ALLGATHER_INTRA_ALGORITHM=nb"
testing_env="${testing_env} env=MPIR_CVAR_NEIGHBOR_ALLGATHER_DEVICE_COLLECTIVE=0"
algo_names="gentran_linear"

for algo_name in ${algo_names}; do
    #set the environment
    env="${testing_env} env=MPIR_CVAR_INEIGHBOR_ALLGATHER_INTRA_ALGORITHM=${algo_name}"

    echo "neighb_allgather 4 ${env}" >> ${testlist_cvar}
    env=""
done

######### Add tests for Ineighbor_allgatherv algorithms ###########

#start with the global environment variables
testing_env=${global_env}

#disable device collectives for ineighbor_allgatherv to test MPIR algorithms
testing_env="${testing_env} env=MPIR_CVAR_INEIGHBOR_ALLGATHERV_DEVICE_COLLECTIVE=0"

#test nb algorithms
testing_env="${testing_env} env=MPIR_CVAR_NEIGHBOR_ALLGATHERV_INTRA_ALGORITHM=nb"
testing_env="${testing_env} env=MPIR_CVAR_NEIGHBOR_ALLGATHERV_DEVICE_COLLECTIVE=0"
algo_names="gentran_linear"

for algo_name in ${algo_names}; do
    #set the environment
    env="${testing_env} env=MPIR_CVAR_INEIGHBOR_ALLGATHERV_INTRA_ALGORITHM=${algo_name}"

    echo "neighb_allgatherv 4 ${env}" >> ${testlist_cvar}
    env=""
done

######### Add tests for Ineighbor_alltoall algorithms ###########

#start with the global environment variables
testing_env=${global_env}

#disable device collectives for ineighbor_alltoall to test MPIR algorithms
testing_env="${testing_env} env=MPIR_CVAR_INEIGHBOR_ALLTOALL_DEVICE_COLLECTIVE=0"

#test nb algorithms
testing_env="${testing_env} env=MPIR_CVAR_NEIGHBOR_ALLTOALL_INTRA_ALGORITHM=nb"
testing_env="${testing_env} env=MPIR_CVAR_NEIGHBOR_ALLTOALL_DEVICE_COLLECTIVE=0"
algo_names="gentran_linear"

for algo_name in ${algo_names}; do
    #set the environment
    env="${testing_env} env=MPIR_CVAR_INEIGHBOR_ALLTOALL_INTRA_ALGORITHM=${algo_name}"

    echo "neighb_alltoall 4 ${env}" >> ${testlist_cvar}
    env=""
done

######### Add tests for Ineighbor_alltoallv algorithms ###########

#start with the global environment variables
testing_env=${global_env}

#disable device collectives for ineighbor_alltoallv to test MPIR algorithms
testing_env="${testing_env} env=MPIR_CVAR_INEIGHBOR_ALLTOALLV_DEVICE_COLLECTIVE=0"

#test nb algorithms
testing_env="${testing_env} env=MPIR_CVAR_NEIGHBOR_ALLTOALLV_INTRA_ALGORITHM=nb"
testing_env="${testing_env} env=MPIR_CVAR_NEIGHBOR_ALLTOALLV_DEVICE_COLLECTIVE=0"
algo_names="gentran_linear"

for algo_name in ${algo_names}; do
    #set the environment
    env="${testing_env} env=MPIR_CVAR_INEIGHBOR_ALLTOALLV_INTRA_ALGORITHM=${algo_name}"

    echo "neighb_alltoallv 4 ${env}" >> ${testlist_cvar}
    env=""
done

######### Add tests for Gatherv algorithms ###########

#start with the global environment variables
testing_env=${global_env}

#disable device collectives for gatherv to test MPIR algorithms
testing_env="${testing_env} env=MPIR_CVAR_GATHERV_DEVICE_COLLECTIVE=0"

#test nb algorithms
testing_env="${testing_env} env=MPIR_CVAR_GATHERV_INTRA_ALGORITHM=nb"
testing_env="${testing_env} env=MPIR_CVAR_IGATHERV_DEVICE_COLLECTIVE=0"
algo_names="gentran_linear"

for algo_name in ${algo_names}; do
    for kval in ${kvalues}; do
        #set the environment
        env="${testing_env} env=MPIR_CVAR_IGATHERV_INTRA_ALGORITHM=${algo_name}"

        echo "gatherv 5 ${env}" >> ${testlist_cvar}
        env=""
    done
done

######### Add tests for Alltoallw algorithms ###########

#start with the global environment variables
testing_env=${global_env}

#disable device collectives for alltoallw to test MPIR algorithms
testing_env="${testing_env} env=MPIR_CVAR_ALLTOALLW_DEVICE_COLLECTIVE=0"

#test nb algorithms
testing_env="${testing_env} env=MPIR_CVAR_ALLTOALLW_INTRA_ALGORITHM=nb"
testing_env="${testing_env} env=MPIR_CVAR_IALLTOALLW_DEVICE_COLLECTIVE=0"
algo_names="gentran_blocked gentran_inplace"

for algo_name in ${algo_names}; do
    #set the environment
    env="${testing_env} env=MPIR_CVAR_IALLTOALLW_INTRA_ALGORITHM=${algo_name}"

    echo "alltoallw1 10 ${env}" >> ${testlist_cvar}
    echo "alltoallw2 10 ${env}" >> ${testlist_cvar}
    echo "alltoallw_zeros 1 ${env}" >> ${testlist_cvar}
    echo "alltoallw_zeros 2 ${env}" >> ${testlist_cvar}
    echo "alltoallw_zeros 5 ${env}" >> ${testlist_cvar}
    echo "alltoallw_zeros 8 ${env}" >> ${testlist_cvar}
    env=""
done

########## Add tests for intra-node bcast algorithms ############

#use release gather based intra-node bcast
testing_env="env=MPIR_CVAR_BCAST_POSIX_INTRA_ALGORITHM=release_gather"

testing_env="${testing_env} env=MPIR_CVAR_COLL_SHM_LIMIT_PER_NODE=131072" #128MB
buffer_sizes="16384 32768"
num_cells="2 4"
tree_types="knomial_1 knomial_2"
kvalues="8 64"

for buf_size in ${buffer_sizes}; do
    for num_cell in ${num_cells}; do
        for kval in ${kvalues}; do
            for tree_type in ${tree_types}; do
                #set the environment
                env="${testing_env} env=MPIR_CVAR_BCAST_INTRANODE_BUFFER_TOTAL_SIZE=${buf_size}"
                env="${env} env=MPIR_CVAR_BCAST_INTRANODE_NUM_CELLS=${num_cell}"
                env="${env} env=MPIR_CVAR_BCAST_INTRANODE_TREE_KVAL=${kval}"
                env="${env} env=MPIR_CVAR_BCAST_INTRANODE_TREE_TYPE=${tree_type}"

                echo "bcasttest 10 ${env}" >> ${testlist_cvar}
                echo "bcastzerotype 5 ${env}" >> ${testlist_cvar}
                env=""
            done
        done
    done
done

########## Add tests for intra-node reduce algorithms ############
#use release gather based intra-node reduce
testing_env="env=MPIR_CVAR_REDUCE_POSIX_INTRA_ALGORITHM=release_gather"

testing_env="${testing_env} env=MPIR_CVAR_COLL_SHM_LIMIT_PER_NODE=131072" #128MB
buffer_sizes="16384 32768"
num_cells="2 4"
tree_types="knomial_1 knomial_2"
kvalues="4 8"

for buf_size in ${buffer_sizes}; do
    for num_cell in ${num_cells}; do
        for kval in ${kvalues}; do
            for tree_type in ${tree_types}; do
                #set the environment
                env="${testing_env} env=MPIR_CVAR_REDUCE_INTRANODE_BUFFER_TOTAL_SIZE=${buf_size}"
                env="${env} env=MPIR_CVAR_REDUCE_INTRANODE_NUM_CELLS=${num_cell}"
                env="${env} env=MPIR_CVAR_REDUCE_INTRANODE_TREE_KVAL=${kval}"
                env="${env} env=MPIR_CVAR_REDUCE_INTRANODE_TREE_TYPE=${tree_type}"

                echo "reduce 5 ${env}" >> ${testlist_cvar}
                echo "reduce 10 ${env}" >> ${testlist_cvar}
                echo "red3 10 ${env}" >> ${testlist_cvar}
                echo "red4 10 ${env}" >> ${testlist_cvar}
                env=""
            done
        done
    done
done

########## Add tests for intra-node allreduce algorithms ############

#use release gather based intra-node allreduce
testing_env="env=MPIR_CVAR_ALLREDUCE_POSIX_INTRA_ALGORITHM=release_gather"

testing_env="${testing_env} env=MPIR_CVAR_COLL_SHM_LIMIT_PER_NODE=131072" #128MB
buffer_sizes="16384"
kvalues="4 8"

for buf_size in ${buffer_sizes}; do
    for kval in ${kvalues}; do
        #set the environment
        env="${testing_env} env=MPIR_CVAR_REDUCE_INTRANODE_BUFFER_TOTAL_SIZE=${buf_size}"
        env="${env} env=MPIR_CVAR_REDUCE_INTRANODE_TREE_KVAL=${kval}"

        echo "allred 4 arg=-count=10 ${env}" >> ${testlist_cvar}
        echo "allred2 4 ${env}" >> ${testlist_cvar}
        env=""
    done
done


######### Add tests for Scatterv algorithms ###########

#start with the global environment variables
testing_env=${global_env}

#disable device collectives for scatterv to test MPIR algorithms
testing_env="${testing_env} env=MPIR_CVAR_SCATTERV_DEVICE_COLLECTIVE=0"

#test nb algorithms
testing_env="${testing_env} env=MPIR_CVAR_SCATTERV_INTRA_ALGORITHM=nb"
testing_env="${testing_env} env=MPIR_CVAR_ISCATTERV_DEVICE_COLLECTIVE=0"
algo_names="gentran_linear"

for algo_name in ${algo_names}; do
    for kval in ${kvalues}; do
        #set the environment
        env="${testing_env} env=MPIR_CVAR_ISCATTERV_INTRA_ALGORITHM=${algo_name}"

        echo "scatterv 4 ${env}" >> ${testlist_cvar}
        env=""
    done
done
