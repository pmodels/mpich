git checkout aurora_test

git checkout -b tmp

git branch -f aurora_test main

git checkout aurora_test

git rebase --onto tmp 9373071ca4

git branch -D tmp


# git checkout aurora
# git merge aurora_test
