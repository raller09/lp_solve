@echo off
if exist lp_solve_5.5_vb.zip del lp_solve_5.5_vb.zip
pkzip25 -add lp_solve_5.5_vb @distribsrc.txt
