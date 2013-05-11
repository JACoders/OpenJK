To compile using MINGW32
========================

* install MSYS/MingW32
* install libz using mingw-get "mingw-get install libz"
* copy dinput.h and dsound.h from wine to mingw/include
* CMakeLists.txt: UseInternalZlib -> OFF
 
* mkdir build && cd build
* run "cmake -G "MSYS Makefiles" .."

* to make a debug build (can use with GDB) add -DCMAKE_BUILD_TYPE=DEBUG to cmake command

Status
======

* can fully compile and run both codemp and code(sp)
* not tried to compile codeJK2 yet

Note
====

* copy relevant dlls from mingw/bin 

 
