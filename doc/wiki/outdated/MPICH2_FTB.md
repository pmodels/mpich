MPICH2 has support for
[CiFTS](http://www.mcs.anl.gov/research/cifts/docs/index.php) FTB. The
current list of supported events can be found on the [MPICH2 FTB
events](MPICH2_FTB_events "wikilink") page.

To enable FTB support in MPICH2, add the following flag to configure:

  -
    `--enable-ftb`

If the FTB include files and library are not in the standard locations,
you will need to specify their locations using

  -
    `--with-ftb=`*FTB_INSTALL_DIR*

to indicate that the include files are located in
*FTB_INSTALL_DIR*/include and the library files are located in
*FTB_INSTALL_DIR*/lib (or *FTB_INSTALL_DIR*/lib64). Alternatively,
the flags

  -
    `--with-ftb-include=`*FTB_INCLUDE_DIR*
    `--with-ftb-lib=`*FTB_LIBRARY_DIR*

can be used to specify the include and library directories separately.