@echo off
set SRC=C:\msys64\ucrt64\bin
set DST=release

copy %SRC%\*.dll %DST%
echo DONE
pause