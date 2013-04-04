@echo off
call vu.bat

xcopy/d/y release\jamp.exe w:\game
if errorlevel 4 goto lowmemory 
if errorlevel 2 goto abort 

xcopy/d/y release\jampgamex86.dll w:\game\base\
if errorlevel 4 goto lowmemory 
if errorlevel 2 goto abort 

xcopy/d/y release\cgamex86.dll w:\game\base\
xcopy/d/y release\uix86.dll w:\game\base\

xcopy/d/y release\*.map w:\game

goto exit 

:lowmemory 
echo *********************************************************************
echo could not copy file.  Some one might be running off the net!
echo *********************************************************************
goto exit 

:abort 
echo You pressed CTRL+C to end the copy operation. 
goto exit 

:exit 

