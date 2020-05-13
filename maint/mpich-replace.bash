#! /usr/bin/env bash
##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

git grep $1 * | cut -f1 -d':' | uniq | xargs sed -i "s/\b$1\b/$2/g"
