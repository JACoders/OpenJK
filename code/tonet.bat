del /q release\jk3sp.exe
del /q release\jk3sp.map
del /q release\jk3gamex86.dll
del /q release\jk3gamex86.map

call vu.bat

xcopy/d/y release\jasp.exe w:\game
xcopy/d/y release\jagamex86.dll w:\game
xcopy/d/y release\*.map w:\game