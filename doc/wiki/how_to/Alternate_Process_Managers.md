# Alternate Process Managers

MPICH supports multiple process managers for starting MPI applications. This document provides detailed information about the available process managers and their configuration options.

## Overview

Process managers in MPICH are responsible for launching and managing MPI processes across different computing environments. While Hydra is the default and recommended process manager, MPICH supports several alternatives for specific use cases.

## Available Process Managers

### Hydra (Default and Recommended)

Hydra is the default process management framework that uses existing daemons on nodes (e.g., ssh, pbs, slurm, sge) to start MPI processes. It provides the most comprehensive feature set and best performance.

#### Key Features
- Automatic detection of resource managers (SLURM, PBS, SGE, etc.)
- Support for heterogeneous environments
- Advanced process placement and binding
- Fault tolerance capabilities
- Comprehensive checkpoint/restart support

#### Usage
Hydra is used automatically when you run:
```bash
mpiexec -n <processes> ./your_application
```

#### Configuration
More detailed information on Hydra configuration can be found at [Using the Hydra Process Manager](https://github.com/pmodels/mpich/blob/main/doc/wiki/how_to/Using_the_Hydra_Process_Manager.md).

### gforker

gforker is a simple process manager that creates processes on a single machine by having mpiexec directly fork and exec them.

#### Use Cases
- Single-node development and testing
- Debugging purposes
- Research platforms
- Simple desktop environments

#### Limitations
- Only supports single-node systems
- Limited scalability
- No advanced process management features

#### Configuration
To use gforker, configure MPICH with:
```bash
./configure --with-pm=gforker
```

### Slurm Integration

Slurm is an external process manager not distributed with MPICH. MPICH provides two ways to work with Slurm environments:

#### Option 1: Hydra with Slurm (Recommended)
MPICH's default process manager (Hydra) has native support for Slurm and will automatically detect Slurm environments and use Slurm capabilities. This is the recommended approach as it provides the best performance and features.

#### Option 2: Native Slurm srun
If you want to use the Slurm-provided "srun" process manager directly, configure MPICH with:
```bash
./configure --with-pmi=slurm --with-pm=no
```

**Note**: The "srun" process manager uses an older PMI standard which does not have some of the performance enhancements that Hydra provides in Slurm environments.

## Process Manager Selection

### Automatic Selection
MPICH will automatically select the best process manager based on your environment:
- If Slurm is detected, Hydra will use Slurm capabilities
- If PBS/Torque is detected, Hydra will use PBS capabilities
- For single-node systems, Hydra will use local process launching

### Manual Selection
You can explicitly configure the process manager during MPICH compilation:

```bash
# Use Hydra (default)
./configure --with-pm=hydra

# Use gforker
./configure --with-pm=gforker

# Use external process manager (like Slurm's srun)
./configure --with-pm=no --with-pmi=slurm
```

## Advanced Configuration

### Environment Variables
Process managers can be configured through environment variables:

#### Hydra Variables
- `HYDRA_LAUNCHER` - Specify the launcher (ssh, rsh, fork, etc.)
- `HYDRA_LAUNCHER_EXEC` - Path to the launcher executable
- `HYDRA_HOST_FILE` - Specify a host file

#### Example
```bash
export HYDRA_LAUNCHER=ssh
export HYDRA_HOST_FILE=/path/to/hostfile
mpiexec -n 4 ./application
```

### Process Binding and Placement
Hydra supports advanced process placement:

```bash
# Bind processes to cores
mpiexec -bind-to core -n 4 ./application

# Map processes by socket
mpiexec -map-by socket -n 4 ./application

# Use specific hosts
mpiexec -hosts node1,node2,node3 -n 12 ./application
```

## Integration with Resource Managers

### SLURM
```bash
# Using salloc
salloc -N 4 -n 16
mpiexec ./application

# Using sbatch
sbatch job_script.sh
```

### PBS/Torque
```bash
# Using qsub
qsub -l nodes=4:ppn=4 job_script.pbs
```

### LSF
```bash
# Using bsub
bsub -n 16 mpiexec ./application
```

## Troubleshooting

### Common Issues

1. **Connection failures**: Check SSH keys and firewall settings
2. **Host resolution**: Ensure all nodes can resolve each other's hostnames
3. **Permission issues**: Verify file permissions and access rights

### Debug Options
```bash
# Enable verbose output
mpiexec -verbose -n 4 ./application

# Debug launcher issues
mpiexec -launcher-exec /usr/bin/ssh -n 4 ./application
```

## Performance Considerations

- **Hydra**: Best overall performance and feature set
- **gforker**: Fastest for single-node applications
- **External managers**: Performance depends on the specific implementation

## Migration Guide

### From other MPI implementations
If migrating from other MPI implementations:
1. Most mpiexec options are compatible
2. Hydra provides equivalent or better functionality
3. Check resource manager integration

### Upgrading MPICH versions
- Process manager interfaces are generally stable
- Check release notes for any breaking changes
- Test your specific use cases after upgrading

## References

- [Using the Hydra Process Manager](https://github.com/pmodels/mpich/blob/main/doc/wiki/how_to/Using_the_Hydra_Process_Manager.md)
- [MPICH Installation Guide](../../../installguide/install.pdf)
- Resource manager documentation (SLURM, PBS, etc.)