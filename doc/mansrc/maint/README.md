# Function descriptions

MPIX semantic descriptions
-
Generates descriptions of MPIX functions for an audiance of users unfamiliar with details of MPI or for quick reference by more expirienced users. 

To generate whole new set of semantic descriptions follow all steps. There is an already generated set of definitions, to use those skip to step 4.
1) Install Opencode with model Claude Opus 4.5 through Argo API. 

    On setup details for Opencode: https://anl.app.box.com/notes/1871610644419?s=hxc72dkm0a8mlmo7ownfl4ixwx6iu3ko

2) Insure `doc/mansrc/semantics.adoc` is empty of all tagged MPIX definitions.

3) From mpich directory run `python3 doc/mansrc/maint/mpixExtraction.py`

4) Build as normal with `./autogen.sh --with-doc`

    Finished pages in Asciidoc format will be generated into `doc/mansrc/c`