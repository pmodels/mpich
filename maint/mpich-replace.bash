#! /usr/bin/env bash
##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

git grep $1 * | cut -f1 -d':' | uniq | xargs sed -i "s/\b$1\b/$2/g"
