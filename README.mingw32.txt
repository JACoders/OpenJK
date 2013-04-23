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
* can compile code (sp) but it won't run - garbled text in edit box then "DROPPED"

Note
====

* remove lib prefix from game dlls
* copy relevant dlls from mingw/bin 

 
