@echo off
..\winbuild\x64\Release\gcc\impgen.exe ..\winbuild\x64\Release\mpi\mpich2mpi.dll > ..\winbuild\x64\Release\gcc\mpich2mpigcc.def
..\winbuild\x64\Release\gcc\impgen.exe ..\winbuild\x64\Release\gfortran\fmpich2g.dll > ..\winbuild\x64\Release\gcc\fmpich2gcc.def
x86_64-w64-mingw32-dlltool --dllname mpich2mpi.dll --def ..\winbuild\x64\Release\gcc\mpich2mpigcc.def --output-lib ..\winbuild\x64\Release\gcc\libmpi.a >> ..\make.log
x86_64-w64-mingw32-dlltool --dllname fmpich2g.dll --def ..\winbuild\x64\Release\gcc\fmpich2gcc.def --output-lib ..\winbuild\x64\Release\gcc\libfmpich2g.a >> ..\make.log

REM Building MPI CXX Interface lib
bash -c "x86_64-w64-mingw32-g++ -I ../src/include -I ../src/include/win64 -c ../src/binding/cxx/initcxx.cxx -o ../winbuild/x64/Release/gcc/initcxx.o" >> ..\make.log
x86_64-w64-mingw32-ar rvs ../winbuild/x64/Release/gcc/libmpicxx.a ../winbuild/x64/Release/gcc/initcxx.o >> ..\make.log

del ..\winbuild\x64\Release\gcc\mpich2mpigcc.def
del ..\winbuild\x64\Release\gcc\fmpich2gcc.def
