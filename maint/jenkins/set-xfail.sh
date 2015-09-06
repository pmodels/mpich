#!/bin/zsh

jobname=""
compiler=""
jenkins_configure=""
queue=""
netmod=""

XFAIL_CONF="maint/jenkins/xfail.conf"

#####################################################################
## Initialization
#####################################################################

while getopts ":f:j:c:o:q:m:" opt; do
    case "$opt" in
        j)
            jobname=$OPTARG ;;
        c)
            compiler=$OPTARG ;;
        o)
            jenkins_configure=$OPTARG ;;
        q)
            queue=$OPTARG ;;
        m)
            netmod=$OPTARG ;;
        f)
            XFAIL_CONF=$OPTARG ;;
        \?)
            echo "Invalid option: -$OPTARG" >&2
            exit 1
    esac
done

#####################################################################
## Main (
#####################################################################

if test ! -f "$XFAIL_CONF" ; then
    echo "Cannot find $XFAIL_CONF. No XFAIL will be applied"
    exit 0
fi

XFAILCond() {
    local job="$1"
    local comp="$2"
    local option="$3"
    local nmod="$4"
    local q="$5"

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

    if [[ ! "$q" == "*" ]]; then
        if [[ ! "$queue" == "$q" ]]; then state=1; fi
    fi

    echo "$state"
}

SCRIPT="apply-xfail.sh"
if [[ -f $SCRIPT ]]; then
    rm $SCRIPT
fi

while read -r line; do
    #clean leading whitespaces
    line=$(echo "$line" | sed "s/^ *//g")
    line=$(echo "$line" | sed "s/ *$//g")
    echo $line
    # skip comment line
    if test -x "$line" -o "${line:1}" = "#" ; then
        continue
    fi

    arr=( $(echo $line) )
    if [[ "0" == $(XFAILCond "${arr[1]}" "${arr[2]}" "${arr[3]}" "${arr[4]}" "${arr[5]}") ]]; then
        echo "${arr[@]:5}" >> $SCRIPT
    fi
done < "$XFAIL_CONF"

if [[ -f $XFAIL_CONF ]]; then
    source $SCRIPT
fi

exit 0
