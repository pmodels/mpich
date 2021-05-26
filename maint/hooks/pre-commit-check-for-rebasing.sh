#!/bin/bash
##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

# This hook prevent from pushing a branch that is not rebased on
# latest MPICH main branch.
#
# It fetches the hash of the latest main branch from github and compares
# the local branch. It returns 1 if either the hash of the latest
# main branch does not exist in the local git repo or the branch needs
# rebasing.
#
# To enable this hook, rename this file to "pre-push" and put it
# under .git/hooks

remote="$1"
url="$2"

z40=0000000000000000000000000000000000000000

GITHUB_REPO="pmodels/mpich"
MAIN_BRANCH="main"

# getting the latest main branch hash from github
MAIN_SHA=$(curl -s https://api.github.com/repos/${GITHUB_REPO}/git/ref/heads/${MAIN_BRANCH} | grep sha | sed -e 's/.*: "//g' | sed -e 's/".*//g')
if test "$(git cat-file -t $MAIN_SHA)" != "commit" ; then
    echo "The repo is behind the https://github.com/pmodels/mpich"
    echo "Run git fetch to update"
    exit 1
fi

IFS=' '
while read local_ref local_sha remote_ref remote_sha
do
    if test "${local_sha}" != $z40 ; then
        LOCAL_BASE_SHA=$(git merge-base ${MAIN_SHA} ${local_sha})

        if test "${LOCAL_BASE_SHA}" != "${MAIN_SHA}" ; then
            echo "Your branch need to rebased on latest main branch before push"
            exit 1
        fi
    fi
done

exit 0
