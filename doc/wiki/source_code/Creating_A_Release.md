# Creating A Release

Creating the release has the following steps:

## Check the Release Status

See the page <https://github.com/pmodels/mpich/milestones> for the
relevant release to ensure that all pending bugs have been fixed.

## Ensure that all testing has been performed

This includes both the results of the nightly tests and the special
tests that are used to check that common configuration options continue
to work and that the coding standards have been followed.

TODO: There was a comprehensive checklist of the prerequisites for a
release. That needs to be placed here. Recent releases have not checked
this list. What follows is a partial list

Check all tests. This includes

1.  The major test suites (MPICH, C++, Intel)
2.  The special tests (See
    [1](../testing/Testing_MPICH.md#running-the-special-tests)
    ); these are important for testing that various options and compiler
    choices work
3.  The "random" tests in the nightly test should not indicate any
    problems; these are used to test correct handling of the configure
    options
4.  Ensure that the Debugger interface works; `(configure mpich with
    --enable-debuginfo; make src/mpi/debugger/tvtest)` can be used as a
    start; a check with `totalview` should also be performed (e.g.:
    "totalview mpiexec -a -n 4 src/mpi/debugger/tvtest" and make sure
    that message queue status works fine)
5.  Ensure that there are no performance artifacts, both for
    point-to-point and collective operations.

## Update all necessary files and commit them to the trunk

  - **Update the version number in `maint/version.m4`**. Note: don't
    update the release date, as it will be automatically replaced during
    tarball generation.
  - **\[STABLE RELEASE ONLY\]** Update the ABI
    **`current:revision:age`** string in **`maint/version.m4`**
    Pre-releases have an ABI string of 0:0:0 to avoid getting
    accidentally linked in place of stable libraries.
  - Update the `CHANGES` file. You can find the commits that went in by
    going through the git log information.
      - Before committing `CHANGES`, it is recommended to send
        `CHANGES`, at least, to core@mpich.org in order to make sure
        everything is clear.
  - Update the `RELEASE_NOTES` file. This requires input from everyone,
    and generally requires asking each person (in person) if the current
    restrictions still apply, and if any new ones should be added.
  - Update the `README.vin` if necessary.

## Create a final tarball for the release using the release.pl script

  - Before using release.pl, make sure these tools are ready: latex,
    [doctext](http://web.engr.illinois.edu/~wgropp/projects/software/sowing/)
    and [txt2man](http://mvertes.free.fr/download/) (from
    [freecode](http://freecode.com/projects/txt2man))
      - txt2man can be installed using package manager. For example,


```
apt-get install txt2man (on Ubuntu)
brew install txt2man (on Mac)
```

  - Get the `release.pl` script in `maint` from the trunk and run it as


```
./release.pl --branch=[git_branch_to_use]  --version=[version] --git-repo=[path_to_git_repository]
```

E.g.,

```
./release.pl --branch=master --version=X.Y.Z --git-repo=https://github.com/pmodels/mpich
```

Notes:

  - The release.pl requires specific autotools to run. Even with a newer
    version, you may not be able to run the script. The script did it on
    purpose to test it maintain the correctness of ABI. You will need
    the exact version of autotools to build and release the tarball.
  - While we use tags are in the format "vX.Y.Z", the version format
    should not have the leading "v", so it should be "X.Y.Z".

## Create a release tag

*(NOTE: below are the notes from when we managed our source in SVN and
the project was still called "MPICH2", though they never 100% accurately
reflected reality. Since the switch to [Git](Git.md), we haven't
been following this model quite as closely. Also, some of the
instructions below don't make as much sense in Git. These instructions
are helpful history, but not how we do things now.)*

  - A branch is only created once for a complete release cycle, i.e.,
    `1.0.7rc1`, `1.0.7rc2`, `1.0.7`, `1.0.7p1`, etc., will have just one
    branch. At what point a release branch is to be created is not
    decided yet (e.g., alpha releases, beta releases, RCs, full
    release), but we currently do that at the time of the RC release.
    This essentially means an svn-copy of the trunk to the
    branches/release directory.
  - If this is the first RC for this release cycle, create a branch.
    Branches are named as `mpich-X.Z.Y`, where `X`, `Y`, `Z` are the
    major, minor and revision version numbers.
  - If this is not the first RC, merge all required changes from the
    trunk to the release branch.
  - A tag is created for every release from the branch. Tag is
    essentially an svn-copy of the branch to the tags/release directory.
    Tags are named as `mpich-X.Y.Z[ext]`, where `[ext]` is `a1`/`a2`/..
    (for alpha releases), `rc1`/`rc2`/.. (for RC releases), etc.

**This is how we manage release tags and branches under git:**

  - tag the release version in your local git repository:

```
git tag -a -m "tagging 'vX.Y.Z'" vX.Y.
[COMMIT_HASH] ([COMMIT_HASH] defaults to HEAD if omitted)
```

  - **It is strongly recommended** that you create the release tarball
    locally and completely test it before pushing the tag to the origin
    repository.

```
make testing
```

  - Once the candidate tarball has been thoroughly tested and inspected,
    push the tag to the mpich and hydra repositories:

```
git push mpich tag vX.Y.Z
```

  - Long term release branches are still discouraged at this time,
    though they can easily be created after the fact as long as the tag
    exists. One easy way to do this in git is: `git push origin
    vX.Y.Z:refs/heads/maint-vX.Y.Z`, which will create a new branch on
    the remote "origin" named "maint-vX.Y.Z" which starts at the same
    commit as the tag "vX.Y.Z". Local branches can easily be created to
    track this remote branch in the usual way: `git branch maint-vX.Y.Z
    --track origin/maint-vX.Y.Z ; git checkout maint-vX.Y.Z` (or the
    equivalent shorthand: `git checkout maint-vX.Y.Z`, as long as that
    branch name only exists in exactly one remote).

### Update MPICH website

The static assets for the MPICH website are stored in
<https://git.cels.anl.gov/pmodels/mpich-www>. It is recommended you
clone the repo locally and add your changes. Once done, you can login to
the CELS GCE environment and update the copy served by the website. The
following assume the repo as working directory.

  - Create directory for tarballs


```
mkdir downloads/X.Y.Z
```

  - Copy release tarballs (mpich, hydra, libpmi, mpich-testsuite) into new
    directory, e.g.,


```
cp -v path/to/{mpich,hydra,libpmi,mpich-testsuite}-X.Y.Z.tar.gz downloads/X.Y.Z/
```

  - Copy a shortlog containing changes since the last stable release.
    Shortlog can be generated with, e.g.,

```
git log --no-merges --format="format:[%cd] %s" --date=short v3.4.2..v4.0a2 > shortlog
```

  - For full release update documentation as well: README.txt,
    installguide.pdf, userguide.pdf (with `mpich-[version]-` prefix)


```
mkdir ~/sandbox-X.Y.Z && cd ~/sandbox-X.Y.Z
tar zxvf $REPO/downloads/X.Y.Z/mpich-X.Y.Z.tar.gz
cp ./mpich-X.Y.Z/README  $REPO/downloads/X.Y.Z/mpich-X.Y.Z-README.txt
cp ./mpich-X.Y.Z/doc/installguide/install.pdf  $REPO/downloads/X.Y.Z/mpich-X.Y.Z-installguide.pdf
cp ./mpich-X.Y.Z/doc/userguide/user.pdf  $REPO/downloads/X.Y.Z/mpich-X.Y.Z-userguide.pdf
```

  - Update git remote


```
git add downloads/X.Y.Z/*
git commit
```

  - Update the server copy

```
ssh homes-gce
umask 0002
cd /nfs/pub_html/gce/projects/mpich
git pull
```

  - Update website to reflect new release: downloads page, main page,
    and new page: <https://www.mpich.org/wp-admin/>
      - Create a new announcement post (in Left Box, News & Events).
      - Change the version number in "Settings -\> MPICH Release (at
        bottom of the left)"
  - Also for full release - update the online Manpages. Change the
    "latest" symlink to point to the new version.

```
cp -rf ~/sandbox-X.Y.Z/mpich-X.Y.Z/www $REPO/docs/vX.Y.Z
cd $REPO/docs
rm -f latest
ln -sf vX.Y.Z latest
chgrp -h mpi latest
```

  - Update links at <https://www.mpich.org/documentation/manpages/>

## Mark the milestone complete on GitHub

  - If there are any pending issues, move them to the next release.

## Update any package managers we're responsible for

At the moment, this is just Homebrew. To do this, you need to:

  - Fork the [original](https://github.com/mxcl/homebrew) on github to
    get your own version
      - Yes, this has to be done on GitHub itself
  - Make a branch to make your changes to the MPICH
      - Doing this in master will only bring pain. Don't do it.
  - Make the changes to Library/Formula/mpich2.rb
      - At the moment, this formula is still named mpich2 and can't be
        changed.
      - There is however, an alias to mpich so people can just say that
        they want to install that.
  - Unless there have been any major changes to the way people should
    build MPICH, all this requires is changing the URL for the formula
    to whatever the new URL is.
      - If there have been new changes to the way people do the usual
        configure/make/make install setup, the rest of the script may
        need to change.
      - If there are new options that we should include in the package
        manager versions (don't read "add all new options here"), then
        they can be added.
  - Push your changes to your own fork on GitHub
  - Send a pull-request to the original mxcl repository

## Send out the release announcement to announce@mpich.org and party\! Yay\!
