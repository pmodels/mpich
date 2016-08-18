#! /bin/bash

git grep $1 * | cut -f1 -d':' | uniq | xargs sed -i "s/\b$1\b/$2/g"
