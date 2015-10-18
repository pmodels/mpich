#!/bin/zsh
#
# (C) 2015 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

jobname=""
compiler=""
jenkins_configure=""
n_nodes="1"
ppn="1"
netmod=""

TIMEOUT_CONF="maint/jenkins/multinode/timeout.conf"

#####################################################################
## Initialization
#####################################################################

while getopts ":f:j:c:o:n:p:m:" opt; do
    case "$opt" in
        j)
            jobname=$OPTARG ;;
        c)
            compiler=$OPTARG ;;
        o)
            jenkins_configure=$OPTARG ;;
        n)
            n_nodes=$OPTARG ;;
        p)
            ppn=$OPTARG ;;
        m)
            netmod=$OPTARG ;;
        f)
            TIMEOUT_CONF=$OPTARG ;;
        \?)
            echo "Invalid option: -$OPTARG" >&2
            exit 1
    esac
done

#####################################################################
## Main (
#####################################################################

if test ! -f "$TIMEOUT_CONF" ; then
    echo "Cannot find $TIMEOUT_CONF. No TIMEOUT will be calculated"
    exit 0
fi

TimeoutCond() {
    local job="$1"
    local comp="$2"
    local option="$3"
    local nmod="$4"
    local _n_nodes="$5"
    local _ppn="$6"

    local state=0

    if [[ ! "$job" == "*" ]]; then
        # clean up jobname and do substring match
        if [[ ! "${jobname%%,*}" == *$job* ]]; then state=1; fi
    fi

    if [[ ! "$comp" == "*" ]]; then
        if [[ ! "$compiler" == "$comp" ]]; then state=1; fi
    fi

    if [[ ! "$option" == "*" ]]; then
        if [[ ! "$jenkins_configure" == "$option" ]]; then state=1; fi
    fi

    if [[ ! "$nmod" == "*" ]]; then
        if [[ ! "$netmod" == "$nmod" ]]; then state=1; fi
    fi

    if [[ ! "$_n_nodes" == "*" ]]; then
        if [[ ! "$n_nodes" == "$_n_nodes" ]]; then state=1; fi
    fi

    if [[ ! "$_ppn" == "*" ]]; then
        if [[ ! "$ppn" == "$_ppn" ]]; then state=1; fi
    fi

    echo "$state"
}

while read -r line; do
    #clean leading whitespaces
    line=$(echo "$line" | sed "s/^ *//g")
    line=$(echo "$line" | sed "s/ *$//g")
    # echo $line
    # skip comment line
    if test -x "$line" -o "${line:1}" = "#" ; then
        continue
    fi

    arr=( $(echo $line) )
    if [[ "0" == $(TimeoutCond "${arr[1]}" "${arr[2]}" "${arr[3]}" "${arr[4]}" "${arr[5]}" "${arr[6]}") ]]; then
        echo "${arr[@]:6}"
        exit 0
    fi
done < "$TIMEOUT_CONF"

exit 0
