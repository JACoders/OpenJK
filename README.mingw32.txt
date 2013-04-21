To compile using MINGW32
========================

* install MSYS/MingW32
* install libz using mingw-get "mingw-get install libz"
* copy dinput.h and dsound.h from wine to mingw/include
 
* mkdir build && cd build
* run "cmake -G "MSYS Makefiles" .."

Status
======

* can fully compile and run codemp
* code binary compiles, but not dll(s)

Note
====

* remove lib prefix from game dlls
* copy relevant dlls from mingw/bin 

 
