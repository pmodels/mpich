#! /usr/bin/env bash
##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

skip_words="inout,improbe,nd,alse,ans,parm,parms,stange,faught,gord,hellow,creat,outweights,configury,numer,thrid,offsetp"
skip_files=".git,*.tex,*.bib,*.sty,*.f,confdb/config.*"

opts=(--ignore-words-list="$skip_words" --skip="$skip_files" --write-changes)
filelist=""
all=0
for arg in $@; do
    if [ "$arg" = "-all" ]; then
        all=1
    elif [ "$arg" = "-i" ]; then
        opts+=( --interactive=3 )
    else
        filelist="$filelist $arg"
    fi
done

if [ "$all" = "1" ]; then
    codespell ${opts[@]}
elif [ -n "$filelist" ] ; then
    codespell ${opts[@]} $filelist
else
    echo "Usage: $0 [-i] filelist"
    echo "   or: $0 [-i] -all"
    exit
fi
