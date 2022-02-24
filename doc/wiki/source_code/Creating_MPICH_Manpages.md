# Creating MPICH Manpages  

The man pages are generated using tools from
<http://wgropp.cs.illinois.edu/projects/software/sowing/>

Install sowing tools like follows:

```
./configure --prefix=</path/to/install>
make && make install
export PATH=</path/to/install>/bin:$PATH
export DOCTEXT_PATH=</path/to/install>/share/doctext
```

After installation, run the following in your MPICH source directory.

```
make mandoc && make install
```
