# Alternate Channels and Devices

This document provides comprehensive information about MPICH's communication devices and channels. For most users, the default CH4 device with OFI netmod is recommended. This guide is for advanced users who need specific device configurations.

## Overview

The communication mechanisms in MPICH are called "devices". MPICH supports:

- **CH4** (default and recommended) - Modern device with superior performance
- **CH3** (legacy) - Older device for compatibility purposes
- **Third-party devices** - Released and maintained by other institutes

## Device Selection

MPICH automatically selects the best device based on your system environment. You can also explicitly specify a device during configuration:

```bash
./configure --with-device=ch4:ofi    # CH4 with OFI (recommended)
./configure --with-device=ch4:ucx    # CH4 with UCX
./configure --with-device=ch3:nemesis # CH3 with nemesis (legacy)
```

## CH4 Device (Recommended)

CH4 is the default and recommended device for MPICH. It provides superior performance and supports modern network technologies. For detailed CH4 netmod configuration, see the main README.md.

### Available CH4 Netmods

- **OFI** - Supports libfabric/OpenFabrics Interface (recommended)
- **UCX** - Supports Unified Communication X library

## CH3 Device (Legacy)

The CH3 device is maintained for compatibility with older systems. For detailed CH3 configuration, refer to [Using the CH3 Device](Using_CH3_device.md).

### Available CH3 Channels

- **nemesis** - Default CH3 channel with shared memory optimizations
- **sock** - Traditional TCP sockets-based communication

## Third-Party Devices

Several third-party organizations maintain additional MPICH devices:

- Contact the respective maintainers for support and documentation
- These devices may have specific requirements and limitations
- Refer to their individual documentation for configuration details

## Migration Recommendations

If you're currently using CH3 or considering device options:

1. **New installations**: Use CH4 with OFI netmod (default)
2. **Existing CH3 users**: Consider migrating to CH4 for better performance
3. **Compatibility needs**: CH3 remains available for systems requiring it
4. **Performance critical**: CH4 provides the best performance and newest features

## Performance Considerations

- CH4 generally provides better performance than CH3
- OFI netmod is typically faster than UCX for most workloads
- Shared memory optimizations are available in both CH4 and CH3
- GPU support is better integrated in CH4

## Configuration Examples

```bash
# Default (recommended)
./configure --prefix=/path/to/install

# Explicit CH4 with OFI
./configure --prefix=/path/to/install --with-device=ch4:ofi

# CH4 with UCX
./configure --prefix=/path/to/install --with-device=ch4:ucx

# Legacy CH3 with nemesis
./configure --prefix=/path/to/install --with-device=ch3:nemesis
```

## Support and Documentation

- CH4 documentation: Main README.md
- CH3 documentation: [Using the CH3 Device](Using_CH3_device.md)
- Shell-specific examples: [Using csh and tcsh Shells](Using_csh_tcsh_shells.md)
- General help: MPICH mailing lists and GitHub issues