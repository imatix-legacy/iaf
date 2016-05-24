@echo off
:-  Build iAF Skeleton
set apname=skeleton
set odbcname=skeleton
set model=lite
set database=odbc
iafbuild %1 %2 %3 %4 %5 %6 %7 %8 %9
