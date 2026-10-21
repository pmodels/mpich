# Jenkins

MPICH uses a [Jenkins](https://www.jenkins.io/) server at
<https://jenkins-pmrs.cels.anl.gov/> to test pull requests and to run
nightly tests of the `main` branch. This page describes how to use it as
a contributor: which tests run on a pull request, how to request more
tests, how to read the results, and how to mark a known failure.

The server and the job scripts are maintained by the MPICH team at
Argonne. The Jenkins web pages, including build logs and test results,
can be viewed without logging in. Viewing or changing the configuration
of a job requires an account on the server.

## Tests That Run Automatically

Every time a pull request is opened, reopened, or updated with new
commits, the following checks run on the new head commit:

| Status on the pull request | What it checks |
| -------------------------- | -------------- |
| `contribution-checker`     | The author is a member of the MPICH team or has a contribution agreement on file |
| `whitespace-checker`       | The changes follow the whitespace rules (see [Code Formatting And Cleanup Script](../source_code/Code_Formatting_And_Cleanup_Script.md)) |
| `spellchecker`             | No new spelling errors |
| `mpich-warnings`           | The build produces no new compiler warnings |

These checks do not run the test suite.

## Requesting More Tests

The tests that build MPICH and run the test suite are started on
request. There are two ways to request them.

### In a Pull Request Comment

Add a comment to the pull request with one trigger phrase per line:

```
test:mpich/ch4/ofi
test:mpich/ch3/tcp
```

The tests run on the head commit of the pull request at the time of the
comment. Pushing new commits does not rerun them; add a new comment to
test the new commits. Requesting a test that is still running for the
same pull request replaces the earlier run.

Trigger comments are only accepted from authorized accounts. If your
comment does not start a test, ask a maintainer to trigger it for you.

### In the Pull Request Description

Put the trigger phrase in square brackets anywhere in the pull request
description:

```
[test:mpich/ch4/ofi]
```

Tests requested this way run when the pull request is opened and again
every time new commits are pushed. Use this for a test you want on
every update, and remove the tag when it is no longer needed.

The description may also select which variant of the warnings check
runs, with `[warnings: normal]`, `[warnings: quick]`, or
`[warnings: rapid]`. The default is `normal`.

### Trigger Phrases

| Trigger phrase            | Status on the pull request | What it tests |
| ------------------------- | -------------------------- | ------------- |
| `test:mpich/ch3/tcp`      | `mpich-review-ch3-tcp`     | ch3 device, nemesis channel with the tcp netmod |
| `test:mpich/ch3/sock`     | `mpich-review-ch3-sock`    | ch3 device, sock channel |
| `test:mpich/ch3/most`     | both of the above          | |
| `test:mpich/ch4/ofi`      | `mpich-review-ch4-ofi`     | ch4 device, ofi netmod |
| `test:mpich/ch4/ofi/more` | `mpich-review-ch4-ofi-more` | ch4 device, ofi netmod, additional configurations |
| `test:mpich/ch4/ucx`      | `mpich-review-ch4-ucx`     | ch4 device, ucx netmod |
| `test:mpich/ch4/most`     | `mpich-review-ch4-ofi` and `mpich-review-ch4-ucx` | |
| `test:mpich/ch4/gpu/ofi`  | `mpich-review-ch4-gpu-ofi` | ch4 device, ofi netmod, with GPU support |
| `test:mpich/ch4/gpu/ucx`  | `mpich-review-ch4-gpu-ucx` | ch4 device, ucx netmod, with GPU support |
| `test:mpich/ch4/gpu`      | both GPU tests             | |
| `test:mpich/pmi`          | `mpich-review-pmi`         | Process managers and PMI |
| `test:mpich/abi`          | `mpich-review-abi`         | Builds with the MPI ABI, the MPICH ABI, and both |
| `test:mpich/doc`          | `mpich-review-doc`         | Documentation build |
| `test:mpich/custom`       | `mpich-review-custom`      | A configuration you specify; see [Custom Tests](#custom-tests) |

The automatic checks can be rerun with `test:mpich/authorship`,
`test:mpich/whitespace`, `test:mpich/spellcheck`, and
`test:mpich/warnings`.

Each test builds and tests several configurations, such as different
configure options or platforms. The page of each job on the Jenkins
server shows the configurations it currently runs. The ch3 tests also
cover macOS and FreeBSD; the other tests run on Linux.

### Choosing Tests

- For a change in one device or netmod, run the test for that device or
  netmod.
- For a change that does not depend on the device, such as a change in
  the process manager or in the MPI layer, one test is usually enough.
  `test:mpich/ch3/tcp` is a good choice because it covers the most
  platforms.
- For a change to PMI or to how processes are launched, add
  `test:mpich/pmi`.
- For a change that touches GPU code paths, add `test:mpich/ch4/gpu`.

The tests share a limited pool of machines. A full test takes from
about 40 minutes (`mpich-review-ch3-tcp`) to over an hour
(`mpich-review-ch4-ofi`), so request only the tests your change needs.

## Reading the Results

Each test reports a status on the commit it tested, shown at the bottom
of the pull request page:

- **pending**: the build has started.
- **success**: every configuration built and all tests passed.
- **failure**: the build failed, or at least one test failed.

The "Details" link opens the Jenkins page for the build, which lists
the failed tests and their output for each configuration.

A status belongs to a commit. After you push new commits, the statuses
of the comment-triggered tests no longer appear, because the new head
commit has not been tested.

## Custom Tests

`test:mpich/custom` runs a single configuration that you describe in the
lines following the trigger phrase. This is useful for reproducing a
failure in one configuration, or for testing a configuration that the
standard tests do not cover.

```
test:mpich/custom debug_vci
netmod: ch4:ofi
compiler: gnu
config: debug
--enable-thread-cs=per-vci
--with-ch4-max-vcis=8
env: MPIR_CVAR_CH4_NUM_VCIS=4
testlist: threads/pt2pt
```

An optional word after the trigger phrase names the run. The status is
then reported as `custom-<word>`, so several custom tests can be
reported side by side on one pull request.

The lines that follow may be:

| Line                           | Meaning |
| ------------------------------ | ------- |
| `netmod: <device>`             | Device and netmod: `ch3:tcp`, `ch3:sock`, `ch4:ofi`, or `ch4:ucx` |
| `compiler: <name>`             | Compiler, for example `gnu`, `clang`, or `intel` |
| `config: <name>`               | A named configuration; see [Configuration Names](#configuration-names) |
| `pm: <name>`                   | Process manager, for example `hydra` or `gforker` |
| `--enable-...`, `--with-...`, `--disable-...`, `--without-...` | An extra configure option. Put each option on its own line |
| `env: <NAME>=<value>`          | An environment variable to set for the build and the tests |
| `testlist: <dir>`              | Run only the tests in this directory of `test/mpi` |
| `testlist: <dir>/<test> <np>`  | Run only this test, written as it would appear in the testlist file |
| `xfail: <testlist file> <pattern>` | Mark the tests whose testlist line starts with the pattern as expected failures |

Without `testlist:` lines the whole test suite runs.

In a pull request description, put the entire request inside the square
brackets:

```
[test:mpich/custom
netmod: ch4:ucx
config: asan
]
```

## Configuration Names

The Jenkins jobs refer to sets of configure options and runtime
settings by name. The names appear in the Jenkins build pages, in
`xfail.conf`, and in the `config:` line of a custom test. The commonly
used ones are:

| Name              | Meaning |
| ----------------- | ------- |
| `default`         | No extra options |
| `debug`           | `--enable-g=all` |
| `strict`          | `--enable-strict` |
| `fast`            | `--enable-fast=all` |
| `nofast`          | `--disable-fast` |
| `noshared`        | `--disable-shared` |
| `noweak`          | `--disable-weak-symbols` |
| `noerrorchecking` | `--disable-error-checking` |
| `nofortran`       | `--disable-fortran` |
| `nocxx`           | `--disable-cxx` |
| `asan`            | `--enable-g=asan` |
| `ubsan`           | `--enable-g=ubsan` |
| `tsan`            | `--enable-g=tsan`, without Fortran |
| `valgrind`        | Runs every test under valgrind |
| `multithread`     | `--enable-threads=multiple`, and tests default to `MPI_THREAD_MULTIPLE` |
| `singlethread`    | `--enable-threads=single --with-thread-package=none` |
| `async`           | `--enable-threads=multiple`, and tests run with `MPIR_CVAR_ASYNC_PROGRESS=1` |
| `vci`             | `--enable-g=mutex --enable-thread-cs=per-vci --with-ch4-max-vcis=8`, and tests run with `MPIR_CVAR_CH4_NUM_VCIS=4` |
| `argobots`, `qthreads` | `--with-thread-package=` the named package |
| `pmi1`, `pmi2`    | `--with-pmi=` the named version |
| `pmix`            | Built with PMIx and launched by an external PMIx launcher |
| `am-only`         | `--enable-ch4-am-only` |
| `no-inline`       | `--enable-ch4-netmod-inline=no --enable-ch4-shm-inline=no` |
| `direct-nm`       | `--without-ch4-shmmods` |
| `nolocal`         | Tests run with `MPIR_CVAR_NO_LOCAL=1` |
| `dataloop`, `yaksa` | `--with-datatype-engine=` the named engine |
| `mpi-abi`         | `--enable-mpi-abi` |
| `mpich-abi`       | `--disable-mpi-abi` |
| `dual-mpi`, `dual-mpich` | `--enable-mpi-abi=dual`, testing the MPI ABI library or the MPICH ABI library |

Some settings apply to most configurations and are worth knowing when
you try to reproduce a failure locally:

- ch4:ofi builds use the embedded libfabric and run with
  `FI_PROVIDER=sockets`.
- Tests run with `MPIR_CVAR_ODD_EVEN_CLIQUES=1`, so that processes on
  one machine communicate through the netmod as well as through shared
  memory.
- XPMEM is disabled.

## Known Failures

A test that is known to fail in some configuration can be marked as an
expected failure (`xfail`), so that it does not hide new failures. Tests
marked this way are skipped and reported as skipped. The rules are
kept in the MPICH source tree, in `test/mpi/maint/jenkins/xfail.conf`,
and are applied by `test/mpi/maint/jenkins/set_xfail.py` before the test
suite runs. Because the file is part of the source tree, a pull request
can add or remove a rule and test the result in the same pull request.

Each rule is one line:

```
<job> <compiler> <config> <netmod> <queue> /<pattern>/ xfail=<reason> <testlist>
```

- The first five fields select the builds the rule applies to: the
  Jenkins job name, the compiler, the configuration name, the netmod,
  and the platform. Each is either `*`, which matches anything, or a
  regular expression that must match at the start of the value. For
  example, `ch3:.*` matches both `ch3:tcp` and `ch3:sock`, and
  `mpich-.*-arm.*` matches the job names that contain `-arm`.
- `/<pattern>/` is a regular expression that selects lines of the
  testlist file.
- `<reason>` is a single word that refers to the issue tracking the
  failure, such as `issue4663`.
- `<testlist>` is the testlist file, relative to `test/mpi`.

For example:

```
* * nofast * *     /^large_acc_flush_local/  xfail=issue4663  rma/testlist
* * * * osx        /^throwtest/              xfail=ticket0    errors/cxx/errhan/testlist
```

The first rule applies to every `nofast` build. The second applies to
every build on macOS.

The build log lists every rule that was applied.

See [Writing Tests](Writing_Tests.md) and
[Testing MPICH](Testing_MPICH.md) for the testlist format and for
running the test suite yourself.

## Nightly Tests

Jenkins also tests the `main` branch every night, on more platforms,
compilers, and configurations than the pull request tests. These jobs
are named `mpich-main-*`.
