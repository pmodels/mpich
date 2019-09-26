set -xe

export MPIR_CVAR_ODD_EVEN_CLIQUES=1
export MPIR_CVAR_CH4_ROOTS_ONLY_PMI=1

for i in `seq 1 10` ; do
    echo "-- $i --"
    mpirun -n 20 ./nonblocking
done
