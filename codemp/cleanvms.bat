set include=
cd game
asm2mak game makefile
make clean
cd ..\cgame
asm2mak cgame makefile
make clean
cd ..\ui
asm2mak ui makefile
make clean
cd ..
