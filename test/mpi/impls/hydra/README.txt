Testing Hydra

The script test_hydra.pl runs following tests:

* process bindings
  Runs tests specified in config/proc_binding.txt. The test is specified as e.g.

    TOPO: topo1.xml
    mpiexec -bind-to user:1,0,4,7 -n 4
        01000000
        10000000
        00001000
        00000001
  
  It runs hydra with the given option under `HYDRA_TOPO_DEBUG=1`. The test
  passes if its debug output matches the expected output.

  If `TOPO: ` line is specified, the specific topology is loaded by setting
  environment variable `HWLOC_XMLFFILE`.

* slurm nodelist parsing
  Runs tests specified in config/slurm_nodelist.txt. The test is given as e.g.
  
    slurm: 4 host-[00-03]
        host-00:1
        host-01:1
        host-02:1
        host-03:1

  It runs the test by setting slurm environment variables including `SLURM_NODELIST`,
  and calls hydra with `-rmk slurm -debug-nodelist true`. The test passes if
  its debug output matches what specified by the test.

