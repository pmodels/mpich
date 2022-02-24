# Jenkins

MPICH has a relatively recent (as of early 2013) [Jenkins continuous
integration](https://wiki.jenkins-ci.org/display/JENKINS/Meet+Jenkins)
server setup at <https://pmrs-jenkins.cels.anl.gov/>. This system runs
automated builds and test runs on regular intervals or when triggered
from a GitHub Pull Request. Configuration for this server is done through
a web interface. This page describes how we use this service in MPICH,
how it works, and how we intend to use it in the future.

## The Details

### Goals

Below are our intended goals by using Jenkins or other 
[continuous integration systems](http://martinfowler.com/articles/continuousIntegration.html)

- Reduce developer time and effort spent running tests by hand
- Reduce developer time spent fiddling with our existing automated
  testing systems
- Tighten the automated testing feedback loop from 1 day to 1 hour or better
- Improve accountability for "breaking the build"
- Improve software (MPICH) quality in several dimensions:
    - Ensure correctness on multiple platforms (Linux, OS X, etc.)
    - Ensure correctness with multiple compilers (GNU, Clang, Intel,
      PGI, etc.)
    - Ensure correctness with multiple configure options and debugging
      levels
    - Prevent performance regressions
- Track historical software quality information further back than just
  "last night"
- Reduce average build times, partly by tracking this information
  historically

Some of this is handled by the existing [Nightly Tests](Nightly_Tests.md "wikilink") infrastructure, though the "old
nightlies" have a number of problems:

- They are fragile. They are a cobbled together series of numerous
  shell scripts run as cron jobs by several different users on the
  team. It can be very confusing to track the entire flow of a test
  run.
- They are not flexible. Adding a new test suite or configuration can
  be difficult.
- They only run nightly.
- They require the MCS NFS system.
- They provide no way to suppress known test issues without completely
  disabling tests or platforms.
- State from one build or day of testing is not always correctly
  cleaned up, leading to false positives and false negatives in some
  cases.

The short term goal is to augment the "old nightlies". In the longer
term it would be good to replace it with an all-Jenkins solution,
provided that it remains stable and can provide us all important
features that are currently offered by the "old nightlies".

### The MPICH Jenkins CI Server and General Jenkins Overview

Visit <https://pmrs-jenkins.cels.anl.gov/> to access the Jenkins server.
You should log in with your ANL username and password.

#### General Overview

After logging in, on the home page you'll see a list of menu options on
the left-hand side with an "executor status" table listed below that. In
the main central/right-hand panel you'll see a list of jobs which you
are able to view.

In order to control what happens in a build, you need to find your way
to the "configure" panel for a given job. If you don't have the right
permissions for the job, any link related to the job will probably yield
an HTTP 404 for you.

#### Terminology
Below is a list of what we consider to be helpful terminology when referring to Jenkins.

- `job` - A logically related set of operations which should be executed
  in order to test a particular piece of software
- `build` - A particular execution of a job
- `workspace` - The working directory where a build executes
- `server` - The Jenkins server which orchestrates builds, reports results,
  and manages configuration
- `build executor` - A host on which builds actually execute. That is, the job
  actions run on that host.
- `build status` - One of STABLE, UNSTABLE, or FAILED

#### Job Naming & Management

To better manage the Jenkins jobs that are created for different
purposes, we define following **standard** views for MPICH-related jobs.
Any new job should be added into the appropriate view. Because Jenkins
does not provide good filter/categorization functionality in the default
**All** view, we also define job naming rules.

- `nightly-master-ch3`
    - Including nightly testing jobs for MPICH master branch / CH3
      channel.
    - The job name is required to start with "mpich-master-ch3-".

- `nightly-master-ch4`
    - Including nightly testing jobs for MPICH master branch / CH4
      channel.
    - The job name is required to start with "mpich-master-ch4-".

- `nightly-tarball`
    - Including tarball jobs for all nightly tests.

- `nightly-abi`
    - Including nightly ABI testing jobs for MPICH master branch and
      stable release.

- `nightly-3.2.x-ch3`
    - Including nightly testing jobs for MPICH stable release / CH3
      channel.
    - The job name is required to start with "mpich-stable-".

- `release-master`
    - Including special testing jobs for MPICH release, e.g., benchmark
      experiments.

- `review-ch3`
    - Including per-commit testing jobs for MPICH master branch / CH3
      channel. It is used for patch review.
    - The job name is required to start with "mpich-review-ch3-".

- `review-ch4`
    - Including per-commit testing jobs for MPICH master branch / CH4
      channel. It is used for patch review.
    - The job name is required to start with "mpich-review-ch4-".

- `weekly`
    - Including weekly jobs for all MPICH branches, e.g. valgrind.


- `maint`
    - Including maitanance related jobs.

- `slurm-test`
    - Including BreadBoard slurm related jobs.

- `external-libs`
    - Including testing jobs for external libraries, e.g., libfabric.

Apart from the above standard views, we have two general views to manage
**temporary** or **private** jobs. All these jobs must be
**self-managed**. Specifically, the job owner is responsible for
deleting it once finished his/her work.

- `temporary`
    - Including any jobs that are temporary used and related to MPICH. For
      example, a temporary job can be created for fixing a bug in MPICH.
    - The job name is required to start with "temp-".


- `private`
    - Including user private jobs for any other purpose.
    - The job name is required to start with `[username]-`, e.g.,
      `jack-` for user Jack's job.

### Build Executor Details

Jenkins utilizes BreadBoard hardware for build testing. All nodes are in
the .mcs.anl.gov domain. The platforms, Jenkins node names, and
hostnames are:

|                                     |             |             |            |            |         |         |         |         |        |
| ----------------------------------- | ----------- | ----------- | ---------- | ---------- | ------- | ------- | ------- | ------- | ------ |
| Ubuntu 12.04 64-bit with IB and MXM | ib64-1      | ib64-2      | ib64-3     | |ib64-4    | ib64-5  | ib64-6  | ib64-7  | ib64-8  | ib64-9 |
| bb93                                | bb73        | bb72        | bb75       | bb66       | bb65    | bb76    | bb87    | bb88    |        |
| ib64-10                             | ib64-11     | ib64-12     | ib64-13    | ib64-14    | ib64-15 | ib64-16 | ib64-17 | ib64-18 |        |
| bb94                                | bb85        | bb74        | bb67       | bb79       | bb80    | bb84    | bb82    | bb68    |        |
| ib64-19                             | ib64-20     | ib64-21     | ib64-22    | ib64-23    | ib64-24 | ib64-25 | ib64-26 |         |        |
| bb90                                | bb91        | bb92        | pending    | bb70       | bb71    | bb77    | bb78    |         |        |
| Ubuntu 12.04 32-bit                 | ubuntu32-1  | ubuntu32-2  | ubuntu32-3 | ubuntu32-4 |         |         |         |         |        |
| bb53                                | bb63        | bb62        | bb64       |            |         |         |         |         |        |
| FreeBSD 9.1 64-bit                  | freebsd64-1 | freebsd64-2 |            |            |         |         |         |         |        |
| bb56                                | bb57        |             |            |            |         |         |         |         |        |
| FreeBSD 9.1 32-bit                  | freebsd32-1 | freebsd32-2 |            |            |         |         |         |         |        |
| bb52                                | bb59        |             |            |            |         |         |         |         |        |
| OSX 10.8.5 64-bit                   | osx-1       | osx-2       | osx-3      |            |         |         |         |         |        |
| mpich-mac1                          | mpich-mac2  | mpich-mac3  |            |            |         |         |         |         |        |
| Solaris 11.3 (x86)                  | solaris-1   |             |            |            |         |         |         |         |        |
| bb58                                |             |             |            |            |         |         |         |         |        |

If for some reason you wanted to log into these machines, use the
'autotest' user. In the /sandbox/jenkins-ci/workspace/ directory you
will find a forest of directories leading you to the configuration
Jenkins displayed. For example,
/mpich-review-tcp/compiler/gnu/jenkins_configure/strict/label/solaris/
has the working directory for the gnu,debug,solaris version.

#### Powercycling

If for some reason a node has become unresponsive and does not return
after a graceful reboot command, the `pm` command can be used from
bblogin to hard powercycle nodes. The format of a `pm` command is, for
example:

```
pm -c bb72
```

### SLURM Cluster

SLURM is an open-source workload manager designed for Linux clusters of
all sizes. The objective is to let SLURM manage all build slaves and
schedule the test jobs that are submitted by Jenkins. The SLURM has
different partitions (queues) for different sets of nodes. All SLURM
partitions are:

|                |                   |
| -------------- | ----------------- |
| Partition      | Nodes             |
| ib64 (default) | ib64-\[1-26\]     |
| ib64-pgi       | ib64-\[17-18\]    |
| ubuntu32       | ubuntu32-\[1-4\]  |
| freebsd64      | freebsd64-\[1-2\] |
| freebsd32      | freebsd32-\[1-2\] |

Note that the **ib64-pgi** partition is only for limiting the number of
concurrent PGI tests.

#### Running Jobs Through SLURM

##### Login & Compiling

The login node is **bblogin.mcs.anl.gov**. You need to use your MCS
account to login. You can also use the login node to compile your codes.
Do not run large, long, multi-threaded, parallel, or CPU-intensive jobs
on the login node.

##### Interactive Jobs

Interactive jobs can run on compute nodes. You can start interactive
jobs to run shell on a compute node.

```
srun --exclusive -N1 --pty bash
```
This will submit an interactive job that requires one dedicated node to
the default partition (ib64). Once the resource is available, the a bash
will be started on the allocated node and available for your use.

The default time limit of a job on SLURM is set to 2 hours. In case more
time is needed for the interactive job, please set it using the **-t**
option. The following example starts an interactive job with a time
limit of 3 hours.

```
srun --exclusive -t 3:00:00 -N1 --pty bash
```

To start a job on a partition other than **ib64**, please set it using
the **-p** option. The following example starts an interactive job on
ubuntu32 partition.

```
srun --exclusive -p ubuntu32 -N1 --pty bash
```

To quit your interactive job:

```
exit
```

Note that the `--exclusive` option ensures the node is only used by
the interactive job.

##### For FreeBSD Nodes

Since FreeBSD nodes do not have MCS accounts and do not support
`--pty`, we need an alternative way for interactive session on FreeBSD
node. The following example is using `salloc` to obtain resource
allocation on a FreeBSD node and start SSH session with helper script
**slurm-ssh**. You can run this one-liner with any account. It will
login to the FreeBSD node as **autotest** after the resource allocation
is granted.

```
salloc -p freebsd64 -t 160 slurm-ssh
```

##### Batch Jobs

To run a batch job, you need to create a SLURM job script.

```
#!/bin/bash
#SBATCH -N 2
#SBATCH -n 20
#SBATCH -t 10:00
#SBATCH -p ib64

mpiexec -n20 ./cpi
exit 0`
```

Once the script is created, you can submit it:

```
sbatch <job_script_name>
```

For both interactive jobs and batch jobs, you can specify the number of
nodes `-N` and the number of processes `-n`. You processes will be
evenly distributed on the allocated nodes. Use `-t` option to set the
time limit of your job. Use `-p` to select the partition.

### Jenkins nightly jobs

The "nightly" jobs are triggered when there is an update in the
**master** branch before midnight. In the "nightly" view, there are some
dependencies between jobs. The dependency means that some jobs are
triggered when the dependent upstream job is successfully completed. The
following illustrates dependencies between jobs:

```
mpich-master-tarball
    ## common jobs across channels
        --> mpich-master-abi-prolog --> mpich-master-abi
    ## jobs testing CH3 channel
        --> armci-mpi-master-ch3
        --> mpich-master-ch3-coverity
        --> mpich-master-ch3-freebsd
        --> mpich-master-ch3-mxm
        --> mpich-master-ch3-ofi
        --> mpich-master-ch3-osx
        --> mpich-master-ch3-portals4
        --> mpich-master-ch3-solaris
        --> mpich-master-ch3-special-tests
        --> mpich-master-ch3-ubuntu
        --> mpich-master-ch3-valgrind
    ## jobs testing CH4 channel
        --> armci-mpi-master-ch4
        --> mpich-master-ch4-coverity
        --> mpich-master-ch4-freebsd
        --> mpich-master-ch4-ofi
        --> mpich-master-ch4-osx
        --> mpich-master-ch4-portals4
        --> mpich-master-ch4-solaris
        --> mpich-master-ch4-special-tests
        --> mpich-master-ch4-ubuntu
        --> mpich-master-ch4-ucx
        --> mpich-master-ch4-valgrind
```

`A --\> B` indicates that the right job B is dependent on the left job
A. For example, **mpich-master-abi-prolog** depends on
**mpich-master-tarball**, and its build is triggered only when the build
of **mpich-master-tarball** is successfully done.

**mpich-master-tarball** creates a tarball of the MPICH master branch
using `release.pl`, and all downstream jobs, which are dependent on
**mpich-master-tarball**, use the tarball. Therefore, the MPICH master
repository is pulled once in **mpich-master-tarball**, and `autogen.sh`
is not executed in most jobs except **mpich-master-tarball**.

### Pull Request Testing Details

There are several jenkins jobs that can be triggered from a pull request
on the MPICH github repo. Jobs are triggered by using a special phrase
in the comments on the pull request. Only comments by members of the
pmodels github organization will be scanned for comment phrases. Note:
the comment is not the same thing as the pull request description. Issue
the pull request, then make a comment on it to trigger the build.

For **CH3** testing:

- The following phrases trigger `mpich-review-ch3-tcp`

```
test:mpich/ch3/all
test:mpich/ch3/most
test:mpich/ch3/tcp
```

- The following phrases trigger `mpich-review-ch3-mxm`

```
test:mpich/ch3/all
test:mpich/ch3/most
test:mpich/ch3/mxm
```

- The following phrases trigger `mpich-review-ch3-portals4`

```
test:mpich/ch3/all
test:mpich/ch3/portals4
```

- The following phrases trigger `mpich-review-ch3-sock`

```
test:mpich/ch3/all
test:mpich/ch3/most
test:mpich/ch3/sock
```

- The following phrases trigger `mpich-review-ch3-ofi`

```
test:mpich/ch3/all
test:mpich/ch3/ofi
```

- The following trigger `mpich-review-ch3-armci-mpi` with the
  default `ch3:nemesis:tcp` netmod of MPICH:

```
test:mpich/ch3/all
test:mpich/ch3/armci-mpi
```

For **CH4** testing:

- The following phrases trigger `mpich-review-ch4-ofi`

```
test:mpich/ch4/all
test:mpich/ch4/most
test:mpich/ch4/ofi
```

- The following phrases trigger `mpich-review-ch4-ucx`

```
test:mpich/ch4/all
test:mpich/ch4/most
test:mpich/ch4/ucx
```

- The following phrases trigger `mpich-review-ch4-uti`

```
test:mpich/ch4/all
test:mpich/ch4/uti
```

- The following phrases trigger `mpich-review-ch4-portals4` \[currently disabled\]

```
test:mpich/ch4/portals4
```

- The following phrases trigger `mpich-review-ch4-armci-mpi` with
  the `ch4:ofi` netmod of MPICH:

```
test:mpich/ch4/all
test:mpich/ch4/armci-mpi
```

### Scripts for Jenkins

In order to ensure the consistency of the scripts and the clarity of
their history, the scripts for Jenkins job scripts are maintained in a
[dedicated repo](https://github.com/pmodels/mpich-jenkins-scripts).
If you need to change these scripts, please follow the git workflow of MPICH
and have someone else review your patch before pushing it.

#### Build Scripts

There is a set of build scripts for existing Jenkins jobs:

```
test-worker-abi-prolog.sh - mpich-master-abi-prolog
test-worker-abi.sh        - mpich-master-abi
test-worker-tarball.sh    - mpich-tarball
test-worker-armci.sh      - All armci jobs
test-worker.sh            - All other jobs
```
`test-worker.sh` is the general script for most of the test jobs.
It takes different configuration and runtime options to run different
tests. **Before adding a dedicated script for a specific configuration
or runtime option, you should consider adding the option to
test-worker.sh.**

#### Jenkins Bootstrap Script

The current setup of Jenkins requires a bootstrap script to invoke the
test-worker scripts in the a SLURM node. All Jenkins jobs is started
checking git repo to a local directory in /sandbox on bblogin1 or
bblogin2. The bootstrap script is responsible for four things:

1.  creating workspace on SLURM node.
2.  copying work repo from bblogin node to SLURM node.
3.  starting test-worker script (srun)
4.  cleanup upon completion

The bblogin node will obtain the SLURM allocation and run the bootstrap
script using **salloc**. The reason for using such a convoluted way to
start job is to avoid using NFS (home directory) as the workspace, which
has been proven to be problematic in Jenkins.

It is recommended to create a new job by copying an existing one in
Jenkins. In this way, the new job will automatically copy the following
bootstrap script. The only revisions needed for the new job are:

1.  line 6: BUILD_SCRIPT
2.  line 15: BUILD_SCRIPT options
3.  line 25: SLURM job options

```
1 : #!/bin/zsh -xe
2 :
3 : cat > job.sh << "EOF"
4 : #!/bin/zsh -xe
5 :
6 : BUILD_SCRIPT="./jenkins-scripts/test-worker.sh"
7 : TARBALL="mpich.tar"
8 :
9 : tar --exclude=${TARBALL} -cf ${TARBALL} * .*
10: REMOTE_WS=$(srun --chdir=/tmp mktemp -d /sandbox/jenkins.tmp.XXXXXXXX)
11: sbcast ${TARBALL} "$REMOTE_WS/${TARBALL}"
12: srun --chdir="$REMOTE_WS" tar xf "$REMOTE_WS/$TARBALL" -C "$REMOTE_WS"
13: 
14: srun --chdir="$REMOTE_WS" \
15:    ${BUILD_SCRIPT} -b ${GIT_BRANCH} -h ${REMOTE_WS} -c $compiler -o $jenkins_configure -q ${label} -m mxm
16:     
17: srun --chdir=/tmp rm -rf "$REMOTE_WS"
18: rm ${TARBALL}
19:
20: exit 0
21: EOF
22:
23: chmod +x job.sh
24: 
25: salloc -J "${JOB_NAME}:${BUILD_NUMBER}:${GIT_BRANCH}" -p ${label} -N 1 --nice=1000 -t 120 ./job.sh
```

#### XFAIL Scripts

A set of scripts is provided for setting xfail at build time.

```
set-xfail.sh
xfail.conf
```
The build script invokes the **set-xfail.sh** script which set the xfail
based on the settings in **xfail.conf**. The **xfail.conf** file enables
setting conditional xfails. The syntax of the xfail setting is:

```
[jobname] [compiler] [jenkins_configure] [netmod] [queue] [sed of XFAIL]
```
Currently, it supports five types of conditions:

- `jobname` - The name of the Jenkins jobs, partial matches are
  allowed. For example, mxm matches both mpich-master-mxm and
  mpich-review-mxm.
- `compiler` - The name of compiler. For example, gnu, intel.
- `jenkins_configure` - The option for ./configure. For example,
  default, debug.
- `netmod` - The type of netmod. For example, mxm and portals4.
- `queue` - The type of machine for testing. Available types are
  ib64, ubuntu32, freebsd32, freebsd64, solaris, osx.

Example for xfail.conf:

```
# xfail alltoall tests for all portals4 jobs
portals4 * * * * sed -i "s+\(^alltoall .*\)+\1 xfail=ticket0+g" test/mpi/threads/pt2pt/testlist
# xfail when the job is "mpich-master-mxm" or "mpich-review-mxm", and the jenkins_configure is "debug".
mxm gnu debug * * sed -i "s+\(^alltoall .*\)+\1 xfail=ticket0+g" test/mpi/threads/pt2pt/testlist
```

### Possible Future Uses of Jenkins in MPICH

- Run the other test suites as well (MPICH1, Intel, C++, LLNL I/O)
- Automated performance regression testing, including historical
  performance trend plotting
- Packaging our nightly snapshot tarballs
- Packaging our final release tarballs
- Write a script to filter TAP results for more sophisticated xfail
  criteria, possibly based on machine or test environment (e.g.,
  exclude `bcast2` failures due to MPIEXEC_TIMEOUT on shared
  machines)
- Gate pushes to origin on 100% clean tests
- Automated builds on platforms that are harder to integrate with the
  old nightlies (BG/Q, niagara machines, etc.)
- Multi-machine tests
- Build an [extreme feedback device](http://www.jensjaeger.com/2010/04/extreme-feedback-device-the-batman-lamp/)
- Email notification for mpich-review tests (with test results)
