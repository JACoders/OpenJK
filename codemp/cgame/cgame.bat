echo off

REM del /q vm
REM rww - removed this.. point of makefile is not have to rebuild all of it

mkdir vm

del vm\bg_lib.asm
asm2mak cgame makefile
make cgame

cd vm

mkdir "..\..\base\vm"
copy *.map "..\..\base\vm"
copy *.qvm "..\..\base\vm"

:quit
cd ..
