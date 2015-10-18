#!/bin/zsh
#
# (C) 2015 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

jobname=""
compiler=""
jenkins_configure=""
queue=""
netmod=""

SKIP_TEST_CONF="maint/jenkins/skip_test.conf"
TEST_SUMMARY="test/mpi/summary.junit.xml"

#####################################################################
## Initialization
#####################################################################

while getopts ":f:j:c:o:q:m:s:" opt; do
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
            SKIP_TEST_CONF=$OPTARG ;;
        s)
            TEST_SUMMARY=$OPTARG ;;
        \?)
            echo "Invalid option: -$OPTARG" >&2
            exit 1
    esac
done

#####################################################################
## Main (
#####################################################################

SkipTestCond() {
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

SkipTest() {
    TEST_SUMMARY_DIR=$(dirname $TEST_SUMMARY)
    if [[ -d "$TEST_SUMMARY_DIR" ]]; then
        mkdir -p "$TEST_SUMMARY_DIR"
    fi
    cat > "$TEST_SUMMARY" << "EOF"
<testsuites>
<testsuite failures="0" errors="0" skipped="0" tests="1" date="$date" name="summary_junit_xml">
<testcase name="none"/>
<system-out/>
<system-err/>
</testsuite>
</testsuites>
EOF
}

if [[ -f "$TEST_SUMMARY" ]]; then
    rm "$TEST_SUMMARY"
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
    if [[ "0" == $(SkipTestCond "${arr[1]}" "${arr[2]}" "${arr[3]}" "${arr[4]}" "${arr[5]}") ]]; then
        SkipTest
        exit 0
    fi
done < "$SKIP_TEST_CONF"

exit 0
