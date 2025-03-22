@echo off
if exist lp_solve_5.5_PHP_exe_win32.tar.gz del lp_solve_5.5_PHP_exe_win32.tar.gz
set dir=lp_solve_5.5\extra\PHP
copy ..\man\PHP.htm
if exist lp_solve_5.5_PHP_exe_win32.zip del lp_solve_5.5_PHP_exe_win32.zip
pkzip25 -dir=current --add lp_solve_5.5_PHP_exe_win32.zip php4\*.dll php5_2\*.dll php5_3\*.dll ex.php example?.php lpdemo.php lp_maker.php lp_solve.php PHP.htm README.txt changes
pushd ..\..\..
tar -T %dir%\distribsrc.txt -cvf - | gzip --best > %dir%\lp_solve_5.5_PHP_source.tar.gz
del %dir%\PHP.htm
popd
