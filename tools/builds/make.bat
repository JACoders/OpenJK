set SRCDIR=%1
IF [%SRCDIR%] == [] (
    set SRCDIR=.
)
cmake -G "Visual Studio 12" -D CMAKE_INSTALL_PREFIX=install -D CMAKE_BUILD_TYPE=Release %SRCDIR%
