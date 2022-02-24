1.  New RMA failures in MPICH-3.2 release:
    1.  ./rma/putfence1 occasionally TIMEOUT
        ([jenkins](https://jenkins.mpich.org/view/per-commit/job/mpich-review-tcp/compiler=gnu,jenkins_configure=strict,label=osx/951/testReport/\(root\)/summary_junit_xml/327_____rma_putfence1_4/))
    2.  ./rma/getfence1 occasionally TIMEOUT
        ([jenkins](https://jenkins.mpich.org/view/per-commit/job/mpich-review-tcp/947/compiler=gnu,jenkins_configure=strict,label=osx/testReport/junit/\(root\)/summary_junit_xml/333_____rma_getfence1_4/))
    3.  ./rma/putpscw1 fails with Portals4, maybe related to a known
        Portals4 failure with ./rma/nullpscw
        ([jenkins](https://jenkins.mpich.org/job/mpich-portals4/81/compiler=intel,jenkins_configure=debug,label=ib/testReport/junit/\(root\)/summary_junit_xml/333_____rma_putpscw1_4/))
    4.  <s> mpich-armci has 3 additional failures, started from build
        214 ([jenkins](https://jenkins.mpich.org/job/armci-mpi/214/))
        </s>
    5.  ./rma/put_bottom failed in special tests
        ([jenkins](https://jenkins.mpich.org/job/mpich-master-special-tests/compiler=pathscale,jenkins_configure=async,label=ubuntu64/308/testReport/junit/\(root\)/summary_junit_xml/396_____rma_put_bottom_2/))
    6.  ./rma/put_base failed in special tests
        ([jenkins](https://jenkins.mpich.org/job/mpich-master-special-tests/compiler=absoft,jenkins_configure=async,label=ubuntu64/lastCompletedBuild/testReport/),
        [jenkins](https://jenkins.mpich.org/job/mpich-mxm/lastCompletedBuild/compiler=gnu,jenkins_configure=async,label=ib/testReport/)),
        might be the same issue with put_bottom.