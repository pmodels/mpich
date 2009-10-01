@echo off
Release\impgen.exe ..\lib\mpich2.dll > mpich2gcc.def
REM Release\impgen.exe ..\lib\mpich2d.dll > mpich2gccd.def
Release\impgen.exe ..\lib\mpich2mpi.dll > mpich2mpi.def
REM Release\impgen.exe ..\lib\mpich2mpid.dll > mpich2mpid.def
Release\impgen.exe ..\lib\fmpich2g.dll > fmpich2gcc.def
REM Release\impgen.exe ..\lib\fmpich2gd.dll > fmpich2gccd.def
dlltool --as /usr/bin/as --dllname mpich2.dll --def mpich2gcc.def --output-lib ..\lib\libmpich2.a >> ..\make.log
REM dlltool --dllname mpich2d.dll --def mpich2gccd.def --output-lib ..\lib\libmpich2d.a
dlltool --as /usr/bin/as --dllname mpich2mpi.dll --def mpich2mpi.def --output-lib ..\lib\libmpi.a >> ..\make.log
REM dlltool --dllname mpich2mpid.dll --def mpich2mpid.def --output-lib ..\lib\libmpid.a
dlltool --as /usr/bin/as --dllname fmpich2g.dll --def fmpich2gcc.def --output-lib ..\lib\libfmpich2g.a >> ..\make.log
REM dlltool --dllname fmpich2gd.dll --def fmpich2gccd.def --output-lib ..\lib\libfmpich2gd.a
REM Building MPI CXX Interface lib
bash -c "g++ -I ../src/include -I ../src/include/win32 -c ../src/binding/cxx/initcxx.cxx -o initcxx.o" >> ..\make.log
ar rvs ../lib/libmpicxx.a initcxx.o >> ..\make.log
del mpich2gcc.def
REM del mpich2gccd.def
del mpich2mpi.def
REM del mpich2mpid.def
del fmpich2gcc.def
REM del fmpich2gccd.def
