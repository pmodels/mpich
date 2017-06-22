ext_coll_tests=""

nl='
'
env=""
# reduce tests
algo_ids="1 2"
kvalues="3 5"
for algo_id in ${algo_ids}; do
    for kval in ${kvalues}; do
        env="env=MPIR_CVAR_USE_REDUCE=${algo_id} env=MPIR_CVAR_REDUCE_TREE_KVAL=${kval} ${nl}"
        ext_coll_tests+="reduce 5 ${env}"
        ext_coll_tests+="reduce 10 ${env}"
        ext_coll_tests+="red3 10 ${env}"
        ext_coll_tests+="red4 10 ${env}"
    done
done
# bcast tests
algo_ids="2 3"
kvalues="4"
for algo_id in ${algo_ids}; do
    for kval in ${kvalues}; do
        env="env=MPIR_CVAR_USE_BCAST=${algo_id} "
        if [ ${algo_id} == 1 ]; then
            env+="env=MPIR_CVAR_BCAST_KNOMIAL_KVAL=${kval}"
        else
            env+="env=MPIR_CVAR_BCAST_KARY_KVAL=${kval}"
        fi
        ext_coll_tests+="bcasttest 10 ${env}${nl}"
        ext_coll_tests+="bcast_full 4 timeLimit=600 ${env}${nl}"
        ext_coll_tests+="bcast_min_datatypes 10 timeLimit=1200 ${env}${nl}"
        ext_coll_tests+="bcast_comm_world 10 timeLimit=1200 ${env}${nl}"
        ext_coll_tests+="bcastzerotype 10 ${env}${nl}"
    done
done
# Allreduce tests
algo_ids="1 2"
for algo_id in ${algo_ids}; do
            env="env=MPIR_CVAR_USE_ALLREDUCE=${algo_id}${nl}"
            ext_coll_tests+="allred 4 ${env}"
            ext_coll_tests+="allred 7 ${env}"
            ext_coll_tests+="allred 4 arg=100 ${env}"
            ext_coll_tests+="allredmany 4 ${env}"
            ext_coll_tests+="allred2 4 ${env}"
            ext_coll_tests+="allred3 10 ${env}"
            ext_coll_tests+="allred4 4 ${env}"
            ext_coll_tests+="allred5 5 ${env}"
            ext_coll_tests+="allred5 10 ${env}"
            ext_coll_tests+="allred6 4 ${env}"
            ext_coll_tests+="allred6 7 ${env}"
done
# Add more tests
export ext_coll_tests
