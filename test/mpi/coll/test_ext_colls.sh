ext_coll_tests=""

nl='
'
testing_env="env=MPIR_CVAR_TEST_MPIR_COLLECTIVES=1"

# bcast tests
algo_ids="1 2"
kvalues="3"
for algo_id in ${algo_ids}; do
    for kval in ${kvalues}; do
        env="env=MPIR_CVAR_USE_BCAST=${algo_id} ${testing_env} "
        env+="env=MPIR_CVAR_BCAST_TREE_KVAL=${kval}"
        ext_coll_tests+="bcasttest 10 ${env}${nl}"
        ext_coll_tests+="bcast_full 4 timeLimit=600 ${env}${nl}"
        ext_coll_tests+="bcast_min_datatypes 10 timeLimit=1200 ${env}${nl}"
        ext_coll_tests+="bcast_comm_world 10 timeLimit=1200 ${env}${nl}"
        ext_coll_tests+="bcastzerotype 10 ${env}${nl}"
    done
done
# Add more tests
export ext_coll_tests
