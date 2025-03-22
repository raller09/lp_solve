@echo off
set dir=lp_solve_5.5\xli\xli_ZIMPL
pushd ..\..\..
tar -T %dir%\distribsrc.txt -cvf - | gzip --best > %dir%\lp_solve_5.5_xli_ZIMPL_source.tar.gz
popd
