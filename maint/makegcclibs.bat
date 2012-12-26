@echo off
Release\impgen.exe ..\lib\mpich.dll > mpichgcc.def
REM Release\impgen.exe ..\lib\mpichd.dll > mpichgccd.def
Release\impgen.exe ..\lib\mpichmpi.dll > mpichmpi.def
REM Release\impgen.exe ..\lib\mpichmpid.dll > mpichmpid.def
Release\impgen.exe ..\lib\fmpichg.dll > fmpichgcc.def
REM Release\impgen.exe ..\lib\fmpichgd.dll > fmpichgccd.def
dlltool --as /usr/bin/as --dllname mpich.dll --def mpichgcc.def --output-lib ..\lib\libmpich.a >> ..\make.log
REM dlltool --dllname mpichd.dll --def mpichgccd.def --output-lib ..\lib\libmpichd.a
dlltool --as /usr/bin/as --dllname mpichmpi.dll --def mpichmpi.def --output-lib ..\lib\libmpi.a >> ..\make.log
REM dlltool --dllname mpichmpid.dll --def mpichmpid.def --output-lib ..\lib\libmpid.a
dlltool --as /usr/bin/as --dllname fmpichg.dll --def fmpichgcc.def --output-lib ..\lib\libfmpichg.a >> ..\make.log
REM dlltool --dllname fmpichgd.dll --def fmpichgccd.def --output-lib ..\lib\libfmpichgd.a
REM Building MPI CXX Interface lib
bash -c "g++ -I ../src/include -I ../src/include/win32 -c ../src/binding/cxx/initcxx.cxx -o initcxx.o" >> ..\make.log
ar rvs ../lib/libmpicxx.a initcxx.o >> ..\make.log
del mpichgcc.def
REM del mpichgccd.def
del mpichmpi.def
REM del mpichmpid.def
del fmpichgcc.def
REM del fmpichgccd.def
