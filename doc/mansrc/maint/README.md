# Semantic descriptions

Generates descriptions of MPI functions for an audiance of users unfamiliar with details of MPI or for quick reference by more expirienced users. 

Generating new descriptions
-
1) Install Opencode with model Claude Opus 4.5 through Argo API. On setup details for Opencode: 
https://anl.app.box.com/notes/1871610644419?s=hxc72dkm0a8mlmo7ownfl4ixwx6iu3ko

2) Place MPI Standard Repo Latex in mpich/doc/mansrc/maint named `mpi-standard`

    Note: Preprocessing for the models rely on Latex structural tags and cannot work with raw text as is. 

3) From mpich directory run `python3 doc/mansrc/maint/extraction.py`

4) Build as normal with `./autogen.sh --with-doc`

    Finished pages in Asciidoc format will be generated into `doc/mansrc/c`