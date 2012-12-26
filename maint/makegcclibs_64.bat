@echo off
..\winbuild\x64\Release\gcc\impgen.exe ..\winbuild\x64\Release\mpi\mpichmpi.dll > ..\winbuild\x64\Release\gcc\mpichmpigcc.def
..\winbuild\x64\Release\gcc\impgen.exe ..\winbuild\x64\Release\gfortran\fmpichg.dll > ..\winbuild\x64\Release\gcc\fmpichgcc.def
x86_64-w64-mingw32-dlltool --dllname mpichmpi.dll --def ..\winbuild\x64\Release\gcc\mpichmpigcc.def --output-lib ..\winbuild\x64\Release\gcc\libmpi.a >> ..\make.log
x86_64-w64-mingw32-dlltool --dllname fmpichg.dll --def ..\winbuild\x64\Release\gcc\fmpichgcc.def --output-lib ..\winbuild\x64\Release\gcc\libfmpichg.a >> ..\make.log

REM Building MPI CXX Interface lib
bash -c "x86_64-w64-mingw32-g++ -I ../src/include -I ../src/include/win64 -c ../src/binding/cxx/initcxx.cxx -o ../winbuild/x64/Release/gcc/initcxx.o" >> ..\make.log
x86_64-w64-mingw32-ar rvs ../winbuild/x64/Release/gcc/libmpicxx.a ../winbuild/x64/Release/gcc/initcxx.o >> ..\make.log

del ..\winbuild\x64\Release\gcc\mpichmpigcc.def
del ..\winbuild\x64\Release\gcc\fmpichgcc.def
