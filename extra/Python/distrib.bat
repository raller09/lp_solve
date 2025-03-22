@echo off

if "%1" == "/i" goto install

set dir=lp_solve_5.5\extra\Python
pushd ..\..\..
copy lp_solve_5.5\extra\man\Python.htm %dir%
tar -T %dir%\distribsrc.txt -cvf - | gzip --best > %dir%\lp_solve_5.5_Python_source.tar.gz
del %dir%\Python.htm
popd

rem call %0 /i win32 2.4
rem call %0 /i win32 2.5
rem call %0 /i win32 2.6
rem call %0 /i win-amd64 2.6
call %0 /i win32 2.7
goto done

:install
shift
set installer=lpsolve55-5.5.2.13.%1-py%2.exe

echo.
if exist dist\%installer% goto ok
echo oops dist\%installer% does nog exist !!!
goto done

:ok
if exist lp_solve_5.5_Python%2_exe_%1.zip del lp_solve_5.5_Python%2_exe_%1.zip
pkzip25 --add lp_solve_5.5_Python%2_exe_%1.zip dist\%installer% ..\man\Python.htm README.txt changes
:done

set installer=
