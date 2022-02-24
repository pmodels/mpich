# Managing Commits In Trunk And Release Branches

**NOTE: This is all stale information now that we have transitioned to
[Git](Git.md). This page should be updated or deleted once we
settle on a git workflow that works for us.**

## Release Trees and Branches

- Trunk represents the next major version release of MPICH.
- A tree represents a major version series (such as 1.4.x) and can be
  found in the
  <https://svn.mcs.anl.gov/repos/mpi/mpich2/branches/release>
  directory (the "x" here is a literal - it does not stand for a
  number). As soon as trunk starts tracking the next major version
  series, an svn tree is created to track the current stable major
  version series. Note that the tree does not appear on the
  [roadmap](https://trac.mpich.org/mpich/roadmap) page.
- A branch represents a specific release version (such as 1.4.1) and
  can be found in the
  <https://svn.mcs.anl.gov/repos/mpi/mpich2/branches/release>
  directory. A release branch is created just before an RC release is
  made for that version.

## Commits in Trunk

- The trunk should be a superset of all release related commits. All
  commits should be made to trunk first, and then merged into the
  release branches. An exception to this rule are commits that are
  obviously only related to a specific release (such as updating the
  maint/Version information).

## Merging changes to Release Trees and Branches

- All commits that do not break the ABI string for a stable major
  release, should also be committed into the major release tree for as
  long as the release tree is maintained. For example, any commits to
  trunk that do not break the ABI string of the 1.4.x release series,
  should also be committed to the 1.4.x tree.
- The following steps can be followed for merging changes to a branch
  or tree:

1.  Create the fix in the trunk; test it and check it in. Remember the
    change-number, or it can be found here:
    <https://trac.mpich.org/mpich/timeline>
2.  Follow these steps for the release branch mpich-X.Y.Z:


```
svn co https://svn.mcs.anl.gov/repos/mpi/mpich2/branches/release/mpich2-X.Y.Z

cd mpich-X.Y.Z

svn merge -c <change-number> https://svn.mcs.anl.gov/repos/mpi/mpich2/trunk .

svn commit
```

It is essential that the `svn commit` is applied just as shown - do not
restrict the commit to a single file or directory, and apply it **only**
in the top-level directory. This is true even if an `svn status` show
property changes (an 'M' in the second rather than the first column) for
files unrelated in any way to the files you just merged.

If the commit fails because of changes in `src/pm/hydra/VERSION`, try
the following:

```
svn revert src/pm/hydra/VERSION

svn commit
```

## Tracking Release Versions

The current release roadmap information can be found here:
<https://trac.mpich.org/mpich/roadmap>
