# Include Files in MPICH

The include files in MPICH are designed to provide good separation of
concerns among the various parts of MPICH (at least, that was the
original plan).

In some cases, include files are organized hierarchically to better
isolate each feature. This is particularly true for the
`src/include/mpiimpl.h` file.

## MPICH

### src/include/mpiimpl.h

  - `mpiimpl.h` -
      - `nmpi.h` - Provides definition of NMPI_ routines (allows
        selection of PMPI or MPI for routines used internally)
      - `mpichconfconst.h` - Definitions needed before mpichconf.h
        (e.g., available thread levels)
      - `mpichconf.h` - Created by configure, this describes the
        properties of the build environment and the selection of
        features in MPICH
      - `mpl.h` - ??
      - `mpibase.h` - Some definitions that may also be used alone
        (without `mpiimpl.h`)
      - `mpitypedefs.h` - Define the basic datatypes used within the
        MPICH implementation
      - `mpidpre.h` - Definitions exported from the ADI implementation
        to the MPICH implementation. These are `*only`* the
        definitions required before the rest of `mpiimpl.h` is
        defined.
      - `mpiimplthread.h` - Thread implementation definitions
      - `mpiutil.h` - Additional helper definitions (should some of
        these be in mpibase.h?)
      - `mpidbg.h` - Defines the MPIU_DBG_xxx macros
      - `mpimem.h` - Definitions for internal memory management
      - `mpi_lang.h` - Defines type defs for attribute copying (?)
      - `mpihandlemem.h` - Defines the macros for the encoded MPI
        handles
      - `mpiparam.h` - Parameter handling interface
      - `mpiattr.h` -
      - `mpichtimer.h` -
      - One of the following is included to define the expansion of the
        function enter and exit macros.
          - `mpiu_func_nesting.h`
          - `mpifuncmem.h` -
          - `mpifunclog.h` -
          - `mpitimerimpl.h` -
      - `mpierror.h` -
      - `mpierrs.h` -
      - `mpi_f77interface.h`
      - `mpidpost.h` - Definitions exported from the ADI
        implementation to the MPICH implementation

Comments: This file has grown over time and probably needs to be
refactored. In particular, there are a number of files that should be
combined; other definitions may be best moved out of `mpiimpl.h`.
Finally, some definitions in `mpiimpl.h` are not universally needed,
and should be moved into the appropriate subdirectory.

### ADI3

The following are as defined for the CH3 implementation of ADI3.

#### src/mpid/ch3/include/mpidimpl.h for CH3

  - `mpidimpl.h` - Included ***only*** within the ADI3 implementation
      - `mpichconf.h` - The master configure definition file
      - `mpiimpl.h` - As defined above (note includes `mpidpre.h`
        and `mpidpost.h`)
      - `mpidftb.h` - ?
      - `mpidpkt.h` -
      - `mpidi_ch3_mpid.h` -

#### src/mpid/ch3/channels/sock/include/mpidi_ch3_impl.h

This file should be used only within the particular channel.

  - `mpidi_ch3_impl.h`
      - `mpidi_ch3i_sock_conf.h`
      - `mpidi_ch3_conf.h`
      - `mpidimp.h`
      - `ch3usock.h`

#### src/mpid/ch3/channels/dllchan/include/

The dllchan is a special channel that permits the dynamic loading of
other channels. In addition to providing a runtime option to select the
channel, it provides a strong test of appropriate information
compartmentalization in the channels.

The key issue for the dllchan is that the ADI3 is built with the
`mpidi_ch3_impl.h` include file, but each channel must be built with
that channels `mpidi_ch3_impl.h` file. Those files must be
compatible (not make any inconsistent definitions).
