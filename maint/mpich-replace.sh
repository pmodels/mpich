#! /bin/bash

grep -r $1 * | cut -f1 -d':' | uniq | xargs sed -i "s/$1/$2/g"
