# Version Control System (VCS)

To help get everyone up to speed on the way we use version control, specifically Git
on projects like MPICH, we've put together this Wiki page as a
reference. Some of the content here may be remedial, but please go
through all of it to make sure you're contributing code in the way that
everyone else on the team expects. Useful resources on learning the basics of our
current VCS, git, can be found below.

- [Git Book](https://git-scm.com/book/en/v2)
- [Git For Computer Scientists](http://eagain.net/articles/git-for-computer-scientists/)

## Committing
Below you can find information on what is required for creating a commit in MPICH.

### Commit Content
#### Coding Standard
MPICH has a [coding stadard](Coding_Standards.md) that should be followed for all newly added code,
along with updating any old code.

#### One Idea Per Commit
- Often commits are edits to a single file and only a few lines. Keeping them
around this size makes automated testing much easier. If your commit includes 
features A & B and we have to roll back to fix B, we don't want to lose A as well.

- Code refactoring should be separate from everything else. If you need to change API 
calls, that should be in a completely separate commit.

#### Don't Pollute Code That Isn't Yours
- Try to make the least intrusive version of your change as possible. Don't try to fix
  things like whitespace in the same commit as code changes. If it's
  absolutely necessary to reformat existing code to make it more
  readable, then it should be done in a separate commit. Not doing this 
  makes it hard to separate out who actually wrote the code.

### Commit Message
#### Title
This should be in the format of `<Codebase Section>: <One Line Description>`.
This should be no longer than 50 characters.

#### Description
This section should contain a description of what the changes in the commit
are actually doing. This could be as small as a single line or paragraph. The 
most important part is the description is concise and to the point. Descriptions should
wrap text at 72 characters. Below are some
tips for writing commit descriptions.

- Use a blank line between the first line and the rest of the
  description
- Write a complete description of the code change in the body.
- Use correct punctuation, spelling, and cApItaLizAtIOn
- [Relevant Blog Post](http://tbaggery.com/2008/04/19/a-note-about-git-commit-messages.html)

#### Example
Below is an example commit:

```
Docs: Mediawiki Pages Conversion

The pages for the wiki.mpich.org site were converted to match
the format for github wiki pages, where the site will be migrated
to due to end of life of the mediawiki at ANL.
```
## Git
### Why Git?

Git was chosen over another system due to it having arguably the 
largest slice of the distributed VCS market right now. More of the world 
will know how to use it to , thus being able to interact with our project.
It is also heavily documented as well as having numerous examples and tutorials.

### Using Git

Until January 7, 2013, MPICH used Subversion (SVN) for its version
control system (VCS). Now we use git. This page documents important
information about the use of git within MPICH. Historical information
about the SVN repository [can be found here](SVN.md).

#### Important URLs
| Type                                   | URL                               |
| -------------------------------------- | --------------------------------- |
| writeable clone URL                    | `git@github.com:pmodels/mpich`    |
| read-only clone URL (via git protocol) | `git://github.com/pmodels/mpich`  |
| read-only clone URL (via http)         | <http://github.com/pmodels/mpich> |

**IF YOU CANNOT ACCESS THE GITHUB REPO BUT THINK YOU SHOULD BE ABLE TO,
CONTACT devel@mpich.org SO THAT WE CAN ADD YOU AS A COLLABORATOR**

#### Local Environment Setup

If you do not have the actual `git` tool, you can get that [here](http://git-scm.com/)
or your preferred software package management system (brew, apt, etc).

The next step is to add the following (substituting your name and email)
into your `~/.gitconfig` file:

```
[user]
    name = Joe Developer
    email = joe@example.org

[color]
    diff = auto
    status = auto
    branch = auto
    ui = auto

# optional, but helps to distinguish between changed and untracked files
[color "status"]
    added = green
    changed = red
    untracked = magenta

# optional, but allows git to create 8-character abbreviated hashes, that are 
# "trac-compatible" for automatic link generation in the comments.

[core]
    whitespace = trailing-space,space-before-tab,indent-with-tab
    abbrev = 8

# optional, but allows better view of commits
[alias]
    graph = log --graph --decorate --abbrev-commit --pretty=oneline
```

#### Repository Management
Authorized committers (core MPICH developers) can clone the mpich
repository using:

```
git clone --origin mpich git@github.com:pmodels/mpich --recursive
```

If you do not have access to the writeable repository but think you
should (because you are a core MPICH developer or collaborator), contact
<devel@mpich.org> for access.

Other users can clone the repository using:

```
git clone --origin mpich git://github.com/pmodels/mpich --recursive
```

This will create an `mpich` directory in your `$PWD` that contains a
completely functional repository with all the dependencies and full
project history.

You can see the branches it contains using:

```
git branch -a
* main
  remotes/mpich/HEAD -> mpich/main
  remotes/mpich/main
```

You can also add other repositories to your local clone. If you have
forked the MPICH repo, for example, you can add it using:

```
git remote add <username> git@github.com:raffenet/mpich --fetch
```

Once this is done, you should be able to see branches from both
repositories:

```
git branch -a
* main
  remotes/raffenet/fix-rma-types
  remotes/raffenet/large-count
  remotes/raffenet/main
  remotes/mpich/HEAD -> mpich/main
  remotes/mpich/main
```

Any number of such remote repositories can be added as needed.

#### Dependencies and Submodules

MPICH depends on several external libraries. They are imported as
submodules in the MPICH project repository. Below is the list of the
dependencies of the current master branch.


| Library   | Repository                           | Location in MPICH source |
| --------- | ------------------------------------ | ------------------------ |
| hwloc     | <https://github.com/open-mpi/hwloc>  | `modules/hwloc`          |
| izem      | <https://github.com/pmodels/izem>    | `modules/izem`           |
| Libfabric | <https://github.com/ofiwg/libfabric> | `modules/libfabric`      |
| UCX       | <https://github.com/openucx/ucx>     | `modules/ucx`            |

When using the aforementioned command (`git clone`) to check out the
MPICH code, the `--recursive` option ensures these dependencies are also
checked out and put into their corresponding directories under the MPICH
code directory.

If the `--recursive` option was not used when you initially checking out
the MPICH code, you can always manually pull these submodules with

```
git submodule update --init
```

The use of submodules allows us to easily manage the external libraries
and their versions which MPICH depends on. The MPICH project only needs
to store the hashes of these external project instead of keeping the
entire copies of these libraries in the MPICH repo. When the a upstream
MPICH commit updates these submodules, you will need to run `git
submodule update --recursive` to bring your local copies up-to-date.

##### Possible Problems

If it's been a long time since you updated, you might see a message like
this

```
git submodule update --init --recursive
error: Server does not allow request for unadvertised object 28872b36f01cc8680abaf1ae0850dec5e0c406de
Fetched in submodule path 'src/mpid/ch4/netmod/ucx/ucx', but it did not contain 28872b36f01cc8680abaf1ae0850dec5e0c406de. Direct fetching of that commit failed.
```

That suggests git is trying to find a revision that only exists in our
fork. Maybe things got reorganized while you were off doing something
else. The following command sequence should get everything back on track.

```
git submodule sync
git submodule update --init
```

#### Changes and Commits

Let's see an example on how to make changes to the code base and commit them in git.

Edit an existing file (foo.c) and a new file (bar.c):

```
vim foo.c   # edit an existing tracked file
...
vim bar.c   # edit a new file
...
```

Check the status of your local git clone:

```
git status
# On branch master
# Changes not staged for commit:
#   (use "git add <file>..." to update what will be committed)
#   (use "git checkout -- <file>..." to discard changes in working directory)
#
#       modified:   foo.c
#
# Untracked files:
#   (use "git add <file>..." to include in what will be committed)
#
#       bar.c
no changes added to commit (use "git add" and/or "git commit -a")

git status -s
 M foo.c
 ?? bar.c
```

Notice that git knows that foo.c has been modified, but it is not staged
for a commit. Git does not know about bar.c and lists it as an untracked
file.

You can ask git to start tracking bar.c and stage it for a commit using:

```
git add bar.c
```

Once bar.c is added, git status should reflect that bar.c is tracked and
staged for a commit.

```
git status
# On branch master
# Changes to be committed:
#   (use "git reset HEAD <file>..." to unstage)
#
#       new file:   bar.c
#
# Changes not staged for commit:
#   (use "git add <file>..." to update what will be committed)
#   (use "git checkout -- <file>..." to discard changes in working directory)
#
#       modified:   foo.c
#
```

You can also add foo.c to be staged for a commit:

```
git add foo.c  # This allows git to stage it for a commit

git status
# On branch master
# Changes to be committed:
#   (use "git reset HEAD <file>..." to unstage)
#
#       new file:   bar.c
#       modified:   foo.c
#
```

Once all the files to be committed are staged, you can commit them in
using:

```
git commit -m 'fixed bug in foo.c, using new bar.c to do so'
[master f36baae] fixed bug in foo.c, using new bar.c to do so
 1 file changed, 1 insertion(+)
 create mode 100644 bar.c
```

If you want to commit all tracked files without explicitly staging them,
you can use:

```
git commit -a -m 'fixed bug in foo.c, using new bar.c to do so'
```

At this stage, you now have created a new commit that is only present in
your **local** repository.

##### Commit Hashes

Once you have committed changes to your local repository clone, you can
view this commit in your commit log:

```
git log
commit 200039300bd7ad390a1268a08d9a92f13b2a46a5
Author: Pavan Balaji <balaji@mcs.anl.gov>
Date:   Mon May 6 21:29:46 2013 -0500

    fixed bug in foo.c, using new bar.c to do so
```

Notice the commit hash on the first line of the log message. This hash
represents the actual commit and can be used to reference it.

If you have multiple commits, you can use a single-line format to view
the commits without taking up too much space:

```
git log --format=oneline
200039300bd7ad390a1268a08d9a92f13b2a46a5 fixed bug in foo.c, using new bar.c to do so
1be61fbcad263cffe6dedfb1597be46ccad2b714 Allow f90 mod files to be installed in an arbitrary directory.
3b8afffb24a94858a44c20f6c4fd1ab11838fc1f Add '--with-mpl-prefix=DIR' configure option.
c18320d3188bfc075e04dbcba3e2899451e133ee Fix '--with-openpa-prefix=DIR' so it actually works.
9164d3db668e1f2c0f82cd238599782dfc1fc045 Allow pkgconfig files to be installed in an arbitrary directory.
```

You can also reduce the commit hash size to a more abbreviated format
using:

```
git log --format=oneline --abbrev-commit
20003930 fixed bug in foo.c, using new bar.c to do so
1be61fbc Allow f90 mod files to be installed in an arbitrary directory.
3b8afffb Add '--with-mpl-prefix=DIR' configure option.
c18320d3 Fix '--with-openpa-prefix=DIR' so it actually works.
9164d3db Allow pkgconfig files to be installed in an arbitrary directory.
```

#### Local Branches

To be comfortable with git compared to svn, you need to view your
commits as a graph, rather than a linear order as specified by svn. Each
commit hash is a vertex (I'll use "vertex" and "hash" interchangeably in
the below text). Branching creates a new linear ordering of vertices
which does not affect any other branch.

Some simple steps first. When you cloned your repository, you have a
bunch of remote branches and one local branch (master). Something like
this --

```
git branch -a
* master
  remotes/mpich/HEAD -> mpich/master
  remotes/mpich/master
```

The local "master" branch has a bunch of commits:

```
% git graph --since='4 hours ago'
* 1be61fbc (HEAD, mpich/master, mpich/HEAD, master) Allow f90 mod files to be installed in an arbitrary directory
* 3b8afffb Add '--with-mpl-prefix=DIR' configure option.
* c18320d3 Fix '--with-openpa-prefix=DIR' so it actually works.
* 9164d3db Allow pkgconfig files to be installed in an arbitrary directory.
* 9d291d1e Add ability to customize the PAMI library name
* 2bfe261f Add ability to customize the MPL library name
* 11702389 Add ability to customize the OpenPA library name
* 47ea4740 Add ability to customize the mpich fortran 90 library name
```

You can create a new branch called "foobar" from master and checkout
this new branch using:

```
git branch foobar

git checkout foobar
```

(or `git checkout -b foobar` for short)

Now, foobar is in the same state as master:

```
git graph --since='4 hours ago'
* 1be61fbc (HEAD, mpich/master, mpich/HEAD, master, foobar) Allow f90 mod files to be installed in an arbitrary d
* 3b8afffb Add '--with-mpl-prefix=DIR' configure option.
* c18320d3 Fix '--with-openpa-prefix=DIR' so it actually works.
* 9164d3db Allow pkgconfig files to be installed in an arbitrary directory.
* 9d291d1e Add ability to customize the PAMI library name
* 2bfe261f Add ability to customize the MPL library name
* 11702389 Add ability to customize the OpenPA library name
* 47ea4740 Add ability to customize the mpich fortran 90 library name
```

Notice that "foobar" is listed on the top-most hash together with
"master".

Now, let's say, you make some changes and commit them into this foobar
branch:

```
git graph --since='4 hours ago'
* cc47a7e3 (HEAD, foobar) demo change 2 on branch foobar
* 81b8e89d demo change 1 on branch foobar
* 1be61fbc (mpich/master, mpich/HEAD, master) Allow f90 mod files to be installed in an arbitrary directory.
* 3b8afffb Add '--with-mpl-prefix=DIR' configure option.
* c18320d3 Fix '--with-openpa-prefix=DIR' so it actually works.
* 9164d3db Allow pkgconfig files to be installed in an arbitrary directory.
* 9d291d1e Add ability to customize the PAMI library name
* 2bfe261f Add ability to customize the MPL library name
* 11702389 Add ability to customize the OpenPA library name
* 47ea4740 Add ability to customize the mpich fortran 90 library name
```

Notice that "foobar" has two new hashes \[cc47a7e3\] and \[81b8e89d\]
that do not belong to "master".

Now you can go back to master and make some other changes.

```
git checkout master

... make some changes and commit ...

git graph --all --since='4 hours ago'
* 98fdfcd0 (HEAD, master) demo change 2 on branch master
* bc0f4051 demo change 1 on branch master
| * cc47a7e3 (foobar) demo change 2 on branch foobar
| * 81b8e89d demo change 1 on branch foobar
|/
* 1be61fbc (mpich/master, mpich/HEAD) Allow f90 mod files to be installed in an arbitrary directory.
* 3b8afffb Add '--with-mpl-prefix=DIR' configure option.
* c18320d3 Fix '--with-openpa-prefix=DIR' so it actually works.
* 9164d3db Allow pkgconfig files to be installed in an arbitrary directory.
* 9d291d1e Add ability to customize the PAMI library name
* 2bfe261f Add ability to customize the MPL library name
* 11702389 Add ability to customize the OpenPA library name
* 47ea4740 Add ability to customize the mpich fortran 90 library name
```

Notice that "master" has two new hashes \[bc0f4051\] and \[98fdfcd0\]
that are on a different linear path than the "foobar" branch, though
both of them started at the same vertex \[1be61fbc\].

Now, to merge the changes from one branch to another, you have two
options --

**Option 1:** While you are on the "master" branch, you can do the
following:

```
git merge foobar
```

This will merge the foobar commit hashes to go on top of your "master"
commit hashes.

```
git graph --since='4 hours ago'
*   168ff91d (HEAD, master) Merge branch 'foobar'
|\
| * cc47a7e3 (foobar) demo change 2 on branch foobar
| * 81b8e89d demo change 1 on branch foobar
* | 98fdfcd0 demo change 2 on branch master
* | bc0f4051 demo change 1 on branch master
|/
* 1be61fbc (mpich/master, mpich/HEAD) Allow f90 mod files to be installed in an arbitrary directory.
* 3b8afffb Add '--with-mpl-prefix=DIR' configure option.
* c18320d3 Fix '--with-openpa-prefix=DIR' so it actually works.
* 9164d3db Allow pkgconfig files to be installed in an arbitrary directory.
* 9d291d1e Add ability to customize the PAMI library name
* 2bfe261f Add ability to customize the MPL library name
* 11702389 Add ability to customize the OpenPA library name
* 47ea4740 Add ability to customize the mpich fortran 90 library name
```

At that point a new vertex is created \[168ff91d\] which is an empty
commit, but is just used for merging the two branches.

**Option 2:** You can cherry-pick specific commits from the "foobar"
branch and create new vertices for those on the "master" branch.

```
git cherry-pick 81b8e89d

git cherry-pick cc47a7e3

git graph --all --since='4 hours ago'
* 9a3ab12f (HEAD, master) demo change 2 on branch foobar
* 4778bb7e demo change 1 on branch foobar
* 98fdfcd0 demo change 2 on branch master
* bc0f4051 demo change 1 on branch master
| * cc47a7e3 (foobar) demo change 2 on branch foobar
| * 81b8e89d demo change 1 on branch foobar
|/
* 1be61fbc (mpich/master, mpich/HEAD) Allow f90 mod files to be installed in an arbitrary directory.
* 3b8afffb Add '--with-mpl-prefix=DIR' configure option.
* c18320d3 Fix '--with-openpa-prefix=DIR' so it actually works.
* 9164d3db Allow pkgconfig files to be installed in an arbitrary directory.
* 9d291d1e Add ability to customize the PAMI library name
* 2bfe261f Add ability to customize the MPL library name
* 11702389 Add ability to customize the OpenPA library name
* 47ea4740 Add ability to customize the mpich fortran 90 library name
```

Notice that the "foobar" branch is still pending, but the changes from
it are available as new vertices in the "master" branch as well (though
with different hashes, since they are new commits with the same
content).

#### Updating Your Local Clone With Remote Changes

All changes made to the remote repository need to be explicitly fetched
to your local clone.

The simplest form of fetching remote changes is:

```
git fetch mpich   # Fetches changes on the "mpich" repository
```

This command fetches all update objects (including new commits, new
branches, and new tags), and places them inside your git clone, but does
not affect your working copy to which no changes are made.

To fetch remote changes from an alternative remote repo, such as
**mpich-dev**, you can use:

```
git fetch mpich-dev
```

You can also fetch changes from all remote repos using:

```
git fetch --all
```

The **git fetch** command is pretty safe, in that it does not modify
your local working copy. In fact, some developers even do a **git fetch
--all** inside cron jobs to run nightly.

**git fetch**, however, does not delete local references to objects that
have been deleted on the remote repository. You can force it to do that
using:

```
git fetch --all --prune
```

Once the remote updates have been fetched, you can view the commit graph
to see how your local and remote branches have diverged.

For example, when you cloned the repository, both the local "master"
branch and the remote "mpich/master" branch point to the same vertex (or
commit hash):

```
git graph --since='35 hours ago'
* 939601e0 (HEAD, mpich/master, mpich/HEAD, master) hwloc fix to support c89 builds.
* 259e65db Revert IBM hwloc versioning patch.
* d2072896 Include stdio.h in the configure test for the definition of NULL.
* ea41f80f Include stdio.h in the configure test for getifaddrs.
* 350fe9b8 Simplify C89 header checks.
* 43bd72a4 Die on autoreconf error instead of chugging along.
* 5b7f9166 Warning squash.
```

Now let's say, you made a bunch of changes to your local master branch:

```
git graph --since='35 hours ago'
* 43b4c604 (HEAD, master) local commit #2
* f16ba80e local commit #1
* 939601e0 (mpich/master, mpich/HEAD) hwloc fix to support c89 builds.
* 259e65db Revert IBM hwloc versioning patch.
* d2072896 Include stdio.h in the configure test for the definition of NULL.
* ea41f80f Include stdio.h in the configure test for getifaddrs.
* 350fe9b8 Simplify C89 header checks.
* 43bd72a4 Die on autoreconf error instead of chugging along.
* 5b7f9166 Warning squash.
```

In the meanwhile, suppose someone else pushed some changes to the remote
repository. Now, when you do a **git fetch** on that repository, the
remote **mpich/master** branch will show a diverged path with some other
commits:

```
git graph --all --since='35 hours ago'
* 43b4c604 (HEAD, master) local commit #2
* f16ba80e local commit #1
| * 1be61fbc (mpich/master, mpich/HEAD) Allow f90 mod files to be installed in an arbitrary directory.
| * 3b8afffb Add '--with-mpl-prefix=DIR' configure option.
| * c18320d3 Fix '--with-openpa-prefix=DIR' so it actually works.
| * 9164d3db Allow pkgconfig files to be installed in an arbitrary directory.
| * 9d291d1e Add ability to customize the PAMI library name
| * 2bfe261f Add ability to customize the MPL library name
| * 11702389 Add ability to customize the OpenPA library name
| * 47ea4740 Add ability to customize the mpich fortran 90 library name
| * bff81082 Add missing initializer
| * 384e0138 Allow devices to override the number of handles and indices through mpidpre.h.
| * 59e0d3b2 Fix typo in hwloc's autogen.sh.
| * af11500e Use hwloc's autogen.sh directly.
| * b0ec78c5 Reorder compiler preference.
|/
* 939601e0 hwloc fix to support c89 builds.
* 259e65db Revert IBM hwloc versioning patch.
* d2072896 Include stdio.h in the configure test for the definition of NULL.
* ea41f80f Include stdio.h in the configure test for getifaddrs.
* 350fe9b8 Simplify C89 header checks.
* 43bd72a4 Die on autoreconf error instead of chugging along.
* 5b7f9166 Warning squash.
```

As you can see in the above graph, **master** and **mpich/master** have
diverged on to two different paths, though both of them are based on a
common vertex \[939601e0\].

At this point, your commit hashes are not in a linear order with respect
to the **mpich/master** hashes. Pushing your changes back to the server
is referred to as a non-fast-forward commit and is not allowed for most
users. To push changes back, you must first rebase your local commits on
top of the latest commit on **mpich/master**. You can do this using:

```
git rebase mpich/master
First, rewinding head to replay your work on top of it...
Applying: local commit #1
Applying: local commit #2
```

Once rebased, the order of commit hashes is restored back to linear
order:

```
git graph --all --since='35 hours ago'
* bfe2ae75 (HEAD, master) local commit #2
* 7d29fec4 local commit #1
* 1be61fbc (mpich/master, mpich/HEAD) Allow f90 mod files to be installed in an arbitrary directory.
* 3b8afffb Add '--with-mpl-prefix=DIR' configure option.
* c18320d3 Fix '--with-openpa-prefix=DIR' so it actually works.
* 9164d3db Allow pkgconfig files to be installed in an arbitrary directory.
* 9d291d1e Add ability to customize the PAMI library name
* 2bfe261f Add ability to customize the MPL library name
* 11702389 Add ability to customize the OpenPA library name
* 47ea4740 Add ability to customize the mpich fortran 90 library name
* bff81082 Add missing initializer
* 384e0138 Allow devices to override the number of handles and indices through mpidpre.h.
* 59e0d3b2 Fix typo in hwloc's autogen.sh.
* af11500e Use hwloc's autogen.sh directly.
* b0ec78c5 Reorder compiler preference.
* 939601e0 hwloc fix to support c89 builds.
* 259e65db Revert IBM hwloc versioning patch.
* d2072896 Include stdio.h in the configure test for the definition of NULL.
* ea41f80f Include stdio.h in the configure test for getifaddrs.
* 350fe9b8 Simplify C89 header checks.
* 43bd72a4 Die on autoreconf error instead of chugging along.
* 5b7f9166 Warning squash.
```

#### Handling Rebase Conflicts

Let's assume we have a conflict on one of the files, `foo.c`, while
performing a rebase.

```
git rebase mpich/master
First, rewinding head to replay your work on top of it...
Applying: fixed bug in foo.c, using new bar.c to do so
Using index info to reconstruct a base tree...
M       foo.c
Falling back to patching base and 3-way merge...
Auto-merging foo.c
CONFLICT (content): Merge conflict in foo.c
Failed to merge in the changes.
Patch failed at 0001 fixed bug in foo.c, using new bar.c to do so
The copy of the patch that failed is found in:
    /Users/goodell/scratch/git-wiki-example/.git/rebase-apply/patch

When you have resolved this problem, run "git rebase --continue".
If you prefer to skip this patch, run "git rebase --skip" instead.
To check out the original branch and stop rebasing, run "git rebase --abort".
```

Now we need to resolve the conflict. We can do this in one of two ways:

```
git mergetool
Merging:
foo.c

Normal merge conflict for 'foo.c':
{local}: modified file
{remote}: modified file
Hit return to start merge resolution tool (vimdiff): [I PRESSED ENTER]
... (search for conflict markers, fix conflict)
```

OR:

```
vim foo.c
... (search for conflict markers, fix conflict)

git add foo.c
```

The first option will evaluate the value of `$EDITOR` (or similar) in
your environment and attempt to provide you with a useful mode for your
editor to help you resolve the conflict. In my case, this is a vimdiff
window showing the left, right, base, and working copy versions of the
conflicted file. `git mergetool` will then automatically `git add` the
file if all conflict markers have been removed from the file when you
exit the mergetool editor. The second option is a slightly more manual
version of the first option that looks more like the SVN approach to the
problem.

Once all conflicts have been resolved, we simply continue the rebase
operation:

```
git rebase --continue
Applying fixed bug in foo.c, using new bar.c to do so
```

If you are unable to resolve the conflict and don't want to continue the
rebase, you can abort it using:

```
git rebase --abort
```

This will take you back to the original setup before you started the
rebase.

#### Pushing Changes Back to the Server

Once all your changes have been locally committed, you have fetched all
remote commits and rebased your local commit hashes at the top of the
remote branch, you are ready to push your changes to the server.

The general format for pushing changes is:

```
git push <repo> <local_ref>:<remote_branch>
```

`repo` refers to the repository to which you want to push the changes.
If you want to push changes to the primary repository you cloned, this
will be **mpich**.

`local_ref` refers to a local reference which can be a local branch, a
local tag, or other non-SHA-1 forms of references (e.g., HEAD, HEAD\~3,
etc.).

Alternatively, you can use a local SHA-1 hash (called <local_ref>) for
pushing commits as:

```
git push <repo> <local_ref>:refs/heads/<remote_branch>
```

Either of the above commands will push the commit corresponding to
`local_ref`, and all other commits on which this commit depends (i.e.,
all other commits that come before this commit and have a path to it in
the graph).

`remote_branch` refers to the name of the remote branch to which the
commits are pushed. If `remote_branch` does not exist, a new remote
branch with that name is created. If no `local_ref` is specified, the
remote branch is deleted.

#### Pull Requests

Changes should be submitted in the form of a "pull request". More info on pull requests can be found 
[here](https://help.github.com/categories/collaborating-with-issues-and-pull-requests/).
Some of the more relevant sections for outside collaborators are:

  - [Working With Forks](https://help.github.com/articles/working-with-forks)
  - [Proposing Changes To Your Work With Pull Requests](https://help.github.com/articles/proposing-changes-to-your-work-with-pull-requests)
  - [Creating A Pull Request From A Fork](https://help.github.com/articles/creating-a-pull-request-from-a-fork)

##### Tips for creating a pull request

    - Describe your changes, both in the pull request description and in
      each individual commit message.
        - Describe the underlying issue, whether it be a bug or a new
          feature.
        - Describe the user-visible impact it will have.
    - Separate your changes so that there is one logical change per patch.
        - Each patch should stand on its own merits. If a patch depends on
          a previous one, it is helpful to note that in the commit
          message.
        - Each patch in a series should be able to compile and run. This
          way bugs can be traced down using `git bisect` in the future.

#### Dealing With Development Branches/Repositories

In SVN we had branches in
<https://svn.mcs.anl.gov/repos/mpi/mpich2/branches/dev>, which we would
typically refer to as `dev/FOO`. Many of these branches had restricted
permissions, especially when used to collaborate on a research paper or
with a vendor. Because of git's distributed nature, it is
difficult/impossible to restrict read permissions for a specific branch
within a repository. So these development branches that require special
permissions are put into their own repositories.

For regular development branches that can be world-readable, a new
repository **mpich-dev** is created. You can add this repository to your
local clone using:

```
% git remote add mpich-dev --fetch git@git.mpich.org:mpich-dev.git
Updating mpich-dev
X11 forwarding request failed on channel 0
remote: Counting objects: 213, done.
remote: Compressing objects: 100% (40/40), done.
remote: Total 89 (delta 77), reused 55 (delta 49)
Unpacking objects: 100% (89/89), done.
From git.mpich.org:mpich-dev
 * [new branch]        fix-rma-types -> mpich-dev/fix-rma-types
 * [new branch]        large-count -> mpich-dev/large-count
 * [new branch]        master     -> mpich-dev/master
 * [new branch]        portals-work -> mpich-dev/portals-work
```

That is, we are actually doing two things:

- adding a new git "remote" by the name of `mpich-dev`;
- fetching its content

You will probably then want to create a local branch to track the remote
branch:

```
git branch fix-rma-types mpich-dev/fix-rma-types
git checkout fix-rma-types
```

You can now start hacking away on your local `fix-rma-types` branch.

If you have local branch `foobar` that you want to create a new branch
for in `mpich-dev`:

```
git push mpich-dev foobar:foobar
```

At the current moment we do not have any easy way to list all of the
available repositories unless you have permissions to view the
`gitolite-admin.git` repository. If you think you should have access to
a particular development branch, contact devel@mpich.org or the specific
MPICH core developer with whom you are working. You can list the
repositories to which you already have access by running:

```
ssh git@git.mpich.org info
hello balaji, this is git@caveat running gitolite3 v3.2-13-gf89408a on git 1.7.0.4

 R W C  papers/..*
 R W C  u/CREATOR/..*
 R W    dmem
 R W    gitolite-admin
 R W    mpich
 R W    mpich-benchmarks
 R W    mpich-bgq
 R W    mpich-dev
 R W    mpich-ibm
 R W    review
```

The Pro Git book has more information about 
[working with remotes](http://git-scm.com/book/en/v2/Git-Basics-Working-with-Remotes).

#### Important Dos and Don'ts

Do:

- Update your repository with remote changes using `git fetch --all
  --prune` to fetch remote changes to your local clone, followed by an
  explicit `git rebase` to base your local changes on top of the
  remote changes.

Don't:

- cherry-pick excessively. Cherry-picking should be a rare activity,
  not a frequent one.
- merge just for the sake of merging. If you have a long-running,
  published topic branch, then don't merge from `master` (for
  example), just because "it's been a while". Instead, only merge to
  pick up specific features/fixes that are not suitable for
  cherry-picking. Name this feature/fix in the merge commit message.


#### Managing Access Controls

The git repositories on `git.mpich.org` are hosted at
[MCS](http://www.mcs.anl.gov/) using
[gitolite](https://github.com/sitaramc/gitolite). 
Gitolite has a very informative
[manual](http://sitaramc.github.com/gitolite/master-toc.html), which I
recommend reading if you have questions about the overall setup or
detailed permissions issues.

Access to these repositories is controlled through a git repository that
is also hosted on the same gitolite server. To access it, clone
`git@git.mpich.org:gitolite-admin.git`. This repository contains two
primary parts: a configuration file (`conf/gitolite.conf`) and a
directory full of public SSH keys. The configuration file specifies
which repositories are valid on the server and which users have
particular permissions to access those repositories. At the top of the
configuration file is a nice big comment that explains the basic format
and permissions rules. **If at all in doubt, consult the manual and/or
goodell@ before making a change.** The rules are not hard, but, just
like making firewall rule changes, small mistakes can lead to real
problems. The `keydir` contains files with the format `USERNAME.pub` or
`USERNAME@NUM.pub`. These files should each contain a valid *public* SSH
key for the user given by `USERNAME`.

Permission changes and repository creation are triggered by pushes to
the `gitolite-admin.git` repository. So once you make your changes to
this repository, commit them (`git commit ...`) and then push the
repository back up to `git@git.mpich.org:mpich.git`.

MPICH-core committers can also create repositories by pushing to any
repository that has a name of the format `u/USERNAME/REPONAME` or
`papers/REPONAME`. Permissions for these repositories are managed by the
creator by running `ssh git@git.mpich.org perms ...`. The gitolite
manual has a section explaining this
[command](http://sitaramc.github.com/gitolite/user.html#perms).
This eliminates the need to fiddle with the `gitolite-admin` repository, but
you will then need to use this alternative ssh-based method if
permissions need to be changed.

#### Creating a New Repository

To restrict permissions for a specific development branch, a new
repository may be created. The example below shows how to create a new
repository from the `mpich/master`. Note that these steps need to be
performed by users with administrative privileges on the MPICH Git
system.

First, proceed to create a new repository: on the `gitolite-admin`
repository, edit the `conf/gitolite.conf` file to add an entry for the
new repository, setting the correct permissions (comments on that file
will guide you through). For more information, see the section `Managing
Access Controls` earlier.

Next, if the new repository is named `foo`, all you need are the
following git commands:

```
git remote add foo git@git.mpich.org:foo.git
git checkout master
git push foo master:master
```

#### Overall Workflow

Below is a sample workflow for committing changes to the MPICH github
repository:

1. For each new bug fix or feature, create and checkout a new local
branch (say ticket-1234) based on mpich/master:

```
git checkout -b ticket-1234 mpich/master
```

2. Make changes as one or more commits on this branch.

In commit messages, if your branch is fixing a ticket, you should use
any of the following keywords to make GitHub do magic to automatically
attach commits to issues (if your branch is fixing an issue).

```
Fix `<project>`#1234
Fixes `<project>`#1234
Resolves `<project>`#1234
See `<project>`#1234
<project>`#1234
```

Where project is: `pmodels/mpich`

3. If other users made any changes to the server in the meanwhile, make
sure your changes apply cleanly on them. To do this, you need to:

- Fetch all remote changes:

```
git fetch --all --prune
```

- Move your commits to sit on top of the new changes from other users:

```
git rebase mpich/master /* If this detects any conflicts, it'll tell you how to resolve them */
```

4. Push your changes to your private github fork and open a pull
request to the main MPICH repo. See GitHub docs on how to fork a repo:
<https://help.github.com/articles/fork-a-repo/>

Depending on how your changes are related to, say, the tcp and mxm
netmods, you can test them using the tcp job queue, the mxm job queue,
or both by adding a comment to the pull request. **Note: only repo
admins can trigger Jenkins tests with these phrases.**

```
test:jenkins/ch3/tcp /* Only tested by the ch3:tcp job queue*/
test:jenkins/ch3/mxm /* Only tested by the ch3:mxm job queue*/
test:jenkins/ch3/all /* Tested by all ch3 queues review jobs */
```

The results of the review jobs will be added to the status of the pull
request.

5. Assign your pull request for someone to review.

6. The reviewer should review the patch using the GitHub review
interface. Comments can be added to specific lines of the patch, and
changes requested before the patch is accepted. Updating the branch in
your github fork will automatically update the pull request. Changes
should be re-tested and re-reviewed as necessary.

7. Once the patch is approved, an MPICH maintainer will click the
button to merge the pull request to the target branch in MPICH. Make
sure to add "Approved-by: " tags in the merge commit message to preserve
any review history in the git log.

```
commit cb944baeb07061759f7c22d85704cc9c673056f9
Merge: f7869f67 14f58e1a
Author: Ken Raffenetti <raffenet@users.noreply.github.com>
Date:   Wed Dec 4 13:18:48 2019 -0600

    Merge pull request #4075 from RozanovAnatoliy/hydra2_timeout

    hydra2: Base implementation of timeout functionality

    Approved-by: Ken Raffenetti <raffenet@mcs.anl.gov>
```

8. Delete any local/remote branches you may have.

```
/* Delete local branches */
git checkout master
git branch -D ticket-1234
```

9. Fetch the changes you just pushed to your local master branch:

```
git fetch --all --prune
git rebase mpich/master
```

## VCS History

### SVN History
#### What has been imported?

Much of the history from our previous SVN repository has been migrated
over to git. This includes:

- All trunk history, with commit messages prefixed by "`[svn-rXXXX]`".
  This history lives in the `master` branch, which is the git
  convention corresponding to SVN's `trunk`. The oldest history that
  was present in SVN was 1.0.6, so that's as far back as the git
  history goes.
- All release tags (which are branch-like in SVN), with their history
  squashed down into a single commit. These commit messages have the
  format "`[svn-synthetic] tags/release/mpich2-1.4.1p1`". These
  commits were then tagged with annotated git tags with names like
  "`v1.4.1p1`".

#### Import Process and Caveats

The history was imported by a custom script because the MPICH SVN
repository was more complicated than `git-svn` could handle.
Specifically, the use of <svn:externals> caused a problem. Problem \#1
is that git-svn cannot handle any form of SVN external natively. Problem
\#2 is that our past use of *relative* SVN externals (e.g., for
`confdb`) was *unversioned*. This means that `svn export -r XYZ
$SVN_PATH` (nor the pinned-revision variant, `@XYZ`) would not actually
reproduce the correct working copy at revision `XYZ` if the `confdb`
directory had been changed since `XYZ`. So the script jumped through a
number of hoops in order to provide the *expected* result from `svn
export`. Branch points were computed by hand, rather than attempt to
teach the script to do this.

#### Why Change?

SVN has numerous, well documented deficiencies:

- Branching and merging are nightmares.
- Inspecting history is much more difficult than it is in git.
- Working offline in SVN is limited.
- Performance is slow.

The only three things that SVN had in its favor were inertia (we already
had it installed, with other infrastructure built around it), support
for fine-grained permissions via the MCS "authz" web page, and everyone
basically knows how to use it at this point. Eventually our SVN pain
began to exceed the inertia benefit and MCS Systems provided gitolite in
order to self-administer permissions with finer granularity. The
education issue is unfortunate, but this was an issue that simply must
be overcome every time that a VCS becomes obsolete (it occurred for the
CVS--\>SVN migration).

### CVS-era MPICH history

When we converted to Subversion in November 2007 we simply made a clean
break with history. The scripts and tools at the time simply could not
handle some aspects of our CVS archive. As a result, tools like "git
blame" and "git log" stop with commit
[6a1cbdcfc42](https://github.com/pmodels/mpich/commit/6a1cbdcfc42).

With some manual assistance (basically resolving all CVS modules so that
you have a single cvs directory without any sub-modules) one can get
2015-era 'git cvsimport' to generate a git tree from our historical (and
only internally available to MCS account holders) CVS archive. One can
find such a modified tree
[here](https://xgitlab.cels.anl.gov/robl/mpich-cvs).
One could add it to one's remote repositories like so:

```
git remote add mpich-cvs <https://xgitlab.cels.anl.gov/robl/mpich-cvs.git>
```

We cannot rebase today's git master onto this historical record. If we
were to do so, it would be like rebasing local changes after having
published them: everyone "downstream" would have a giant headache next
time they picked up changes to the master repository. Instead, we can
use
[git-replace](https://www.kernel.org/pub/software/scm/git/docs/git-replace.html).

In our mpich-master tree every commit has a parent commit... except
[6a1cbdcfc42](https://github.com/pmodels/mpich/commit/6a1cbdcfc42). And in
mpich-CVS.git, the main-line tree stops at
[7e0e4d706c32cd73e](https://github.com/pmodels/mpich-cvs/commit/7e0e4d706c32cd73e).
Actually, there are two post-conversion commits to the CVS repository.
We should not use those, but they were made in 2009 so they are easy to
spot. A sufficiently new 'git replace' has --graft, which sounds like
what we want. If one only has git-1.9.1 (as on RobL's Ubuntu-14.04
machine), one must simply stomp on the initial "put everything in its
place" commit

```
git replace 6a1cbdcfc 7e0e4d706c32cd73e
```

Now your working repository has all the MPICH history dating back to the
birth of MPICH2 in 2001. In fact, because we never re-wrote ROMIO but
instead simply carried it forward from MPICH1, you have some history
dating back to the birth of ROMIO in 1998.
